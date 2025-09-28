#include "threads_mgr.h"
#include "cpu.h"
#include "debug.h"
#include "stub.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Threads_Mgr threads_mgr;

#define THREADS_CRASH(str)                                                                                             \
    do {                                                                                                               \
        fprintf(stderr, "Threads manager crashed: %s\n", str);                                                         \
        exit(EXIT_FAILURE);                                                                                            \
    } while (0);

static int thread_init() {
    int i;
    bool stop = true;

    // find the vcore struct to use
    for (i = 1; i < ctx.cores; i++) {
        if (pthread_equal(pthread_self(), GET_THREAD_ID(i)) != 0)
            break;
    }

    while (stop)
        __atomic_load(&threads_mgr.atomic_stop_all, &stop, __ATOMIC_ACQUIRE);

    return i;
}

static void debug_core_run(VCore *core, Halt_Cond *halt_cond) {
    bool stop_all;
    int ret;

    while (1) {
        // check if every core has been stopped (es. breakpoint during execution)
        stop_all = true;

        while (stop_all)
            __atomic_load(&threads_mgr.atomic_stop_all, &stop_all, __ATOMIC_ACQUIRE);

        // after this check someone (possibly the debugging thread) should have
        // placed this core to block on his "halted" condition, until next continue/command

        ret = pthread_mutex_lock(&halt_cond->mutex);
        if (ret != 0)
            THREADS_CRASH("LOCK FAILED")

        while (halt_cond->halted) {
            ret = pthread_cond_wait(&halt_cond->cond, &halt_cond->mutex);
            if (ret != 0)
                THREADS_CRASH("COND WAIT")
        }

        vcore_step(core);

        ret = pthread_mutex_unlock(&halt_cond->mutex);
        if (ret != 0)
            THREADS_CRASH("COND WAIT")
    }
}

static void *core_thread(void *args) {

    int i = thread_init();

    vcore_run(&GET_CORE(i));

    return NULL;
}

static void *debug_core_thread(void *args) {

    int i = thread_init();

    debug_core_run(&GET_CORE(i), &GET_HALT(i));
    return NULL;
}

void threads_mgr_init() {
    int ret;

    // TODO: make them aligned to 64 bytes in order to avoid
    // false sharing
    threads_mgr.threads_cores = malloc(sizeof(Thread) * ctx.cores);
    if (threads_mgr.threads_cores == NULL)
        THREADS_CRASH("OOM");

    threads_mgr.atomic_stop_all = true;
    threads_mgr.halt_cond = NULL;

    for (int i = 0; i < ctx.cores; i++)
        memset(&GET_CORE(i), 0, sizeof(VCore));

    if (!ctx.debug)
        return;

    // set debugging structs
    threads_mgr.halt_cond = malloc(sizeof(Halt_Cond) * ctx.cores);
    if (threads_mgr.halt_cond == NULL)
        THREADS_CRASH("OOM");

    for (int i = 0; i < ctx.cores; i++) {
        threads_mgr.halt_cond->halted = true;
        pthread_mutex_init(&GET_HALT(i).mutex, NULL);
        ret = pthread_cond_init(&GET_HALT(i).cond, NULL);
        if (ret != 0)
            THREADS_CRASH("PTHREAD COND INIT");
    }
}

void threads_mgr_run() {
    int ret;
    if (ctx.debug) {
        // from 1 because the main thread is already core 0
        for (int i = 1; i < ctx.cores; i++) {
            ret = pthread_create(&GET_THREAD_ID(i), NULL, debug_core_thread, NULL);
            if (ret != 0)
                THREADS_CRASH("PTHREAD CREATE")
        }

        // create debug thread
        ret = pthread_create(&threads_mgr.debug_thread, NULL, debug_thread_fun, NULL);
        if (ret != 0)
            THREADS_CRASH("PTHREAD CREATE")

    } else {
        for (int i = 1; i < ctx.cores; i++) {
            ret = pthread_create(&GET_THREAD_ID(i), NULL, core_thread, NULL);
            if (ret != 0)
                THREADS_CRASH("PTHREAD CREATE")
        }
    }

    // run the cores except core 0 that is the actual thread
    __atomic_clear(&threads_mgr.atomic_stop_all, __ATOMIC_RELEASE);

    if (ctx.debug)
        debug_core_run(&GET_CORE(0), &GET_HALT(0));
    else
        vcore_run(&GET_CORE(0));
}
