#include "threads_mgr.h"
#include "cpu.h"
#include "debug.h"
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
    bool finished_step = true;

    int ret;

    while (1) {
        // check if every core has been stopped (es. breakpoint during execution)
        stop_all = true;

        while (stop_all)
            __atomic_load(&threads_mgr.atomic_stop_all, &stop_all, __ATOMIC_ACQUIRE);

        // after this check someone (possibly the debugging thread) should have
        // placed this core to block on his "halted" condition, until next continue/command

		printf("PASSED_STOP_ALL\n");

        ret = pthread_mutex_lock(&halt_cond->mutex);
        if (ret != 0)
            THREADS_CRASH("LOCK FAILED")

        while (halt_cond->halted) {
            ret = pthread_cond_wait(&halt_cond->cond, &halt_cond->mutex);
            if (ret != 0)
                THREADS_CRASH("COND WAIT FAILED")
        }

        printf("STEPPING\n");
        vcore_step(core);

        ret = pthread_mutex_unlock(&halt_cond->mutex);
        if (ret != 0)
            THREADS_CRASH("UNLOCK FAILED")

        __atomic_store(&halt_cond->finished_step, &finished_step, __ATOMIC_RELEASE);
    }
}

static void *core_thread_fun(void *args) {

    int i = thread_init();

    vcore_run(&GET_CORE(i));

    return NULL;
}

static void *debug_core_thread_fun(void *args) {

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
        GET_HALT(i).halted = true;
        GET_HALT(i).finished_step = false;
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
            ret = pthread_create(&GET_THREAD_ID(i), NULL, debug_core_thread_fun, NULL);
            if (ret != 0)
                THREADS_CRASH("PTHREAD CREATE")
        }

        // create debug thread
        ret = pthread_create(&threads_mgr.debug_thread, NULL, debug_thread_fun, NULL);
        if (ret != 0)
            THREADS_CRASH("PTHREAD CREATE")

    } else {
        for (int i = 1; i < ctx.cores; i++) {
            ret = pthread_create(&GET_THREAD_ID(i), NULL, core_thread_fun, NULL);
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

void threads_mgr_halt_all() {
    bool stop = true;

    __atomic_store(&threads_mgr.atomic_stop_all, &stop, __ATOMIC_RELEASE);

    for (int i = 0; i < ctx.cores; i++) {
        pthread_mutex_lock(&GET_HALT(i).mutex);
        GET_HALT(i).halted = true;
        pthread_mutex_unlock(&GET_HALT(i).mutex);
    }

    stop = false;
    __atomic_store(&threads_mgr.atomic_stop_all, &stop, __ATOMIC_RELEASE);
}

void threads_mgr_continue_core(int core_idx) {
    int ret;

    pthread_mutex_lock(&GET_HALT(core_idx).mutex);
    GET_HALT(core_idx).halted = false;
    ret = pthread_cond_signal(&GET_HALT(core_idx).cond);
    if (ret != 0)
        THREADS_CRASH("PTHREAD SIGNAL")
    pthread_mutex_unlock(&GET_HALT(core_idx).mutex);
}

void threads_mgr_continue_all() {
    // at the moment the cores just start one after the other
    // and not at the same time

    for (int i = 0; i < ctx.cores; i++) {
        threads_mgr_continue_core(i);
    }
}

void threads_mgr_step_core(int core_idx) {
    bool stop = true;
    bool finished_step = false;

    // prepare to stop after stepping
    __atomic_store(&threads_mgr.atomic_stop_all, &stop, __ATOMIC_RELEASE);

    // release the core
    threads_mgr_continue_core(core_idx);

    // now the thread should be waiting on atomic_stop_all
	
    printf("GAS\n");
    // wait to finish the step
    while (!finished_step)
        __atomic_load(&GET_HALT(core_idx).finished_step, &finished_step, __ATOMIC_ACQUIRE);

    // prepare the halt
    pthread_mutex_lock(&GET_HALT(core_idx).mutex);
    GET_HALT(core_idx).halted = true;
    pthread_mutex_unlock(&GET_HALT(core_idx).mutex);

    __atomic_clear(&GET_HALT(core_idx).finished_step, __ATOMIC_RELEASE);

    // release the thread that will halt on "halted" condition
    stop = false;
    __atomic_store(&threads_mgr.atomic_stop_all, &stop, __ATOMIC_RELEASE);
}
