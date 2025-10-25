#include "threads_mgr.h"
#include "args.h"
#include "cpu.h"
#include "debug.h"
#include "defs.h"
#include "macros.h"
#include <asm-generic/errno-base.h>
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>

extern Threads_Mgr threads_mgr;

#define pthread_mutex_unlock_(mutex_ref)                                                                               \
    do {                                                                                                               \
        if (pthread_mutex_unlock(mutex_ref) != 0)                                                                      \
            SV32_CRASH("UNLOCK FAILED");                                                                               \
    } while (0)

#define pthread_mutex_lock_(mutex_ref)                                                                                 \
    do {                                                                                                               \
        if (pthread_mutex_lock(mutex_ref) != 0)                                                                        \
            SV32_CRASH("LOCK FAILED");                                                                                 \
    } while (0)

#define __WAIT_IF_SET__(condition_ref)                                                                                 \
    do {                                                                                                               \
        while (__atomic_load_n(condition_ref, __ATOMIC_ACQUIRE)) {                                                     \
        }                                                                                                              \
    } while (0)

#define __RESET_COND__(condition_ref) __atomic_clear(condition_ref, __ATOMIC_RELEASE)

#define __SET_COND__(condition_ref) __atomic_store_n(condition_ref, true, __ATOMIC_RELEASE)

#define __BARRIER_COUNT_WAIT__ barrier_count_wait()

#define __RESET_BARRIER_COUNT__ __atomic_store_n(&threads_mgr.atomic_barrier_count, ctx.cores, __ATOMIC_RELEASE)

// wait until barrier reaches 0, if already 0 just return

void barrier_count_wait(void) {
    if (__atomic_load_n(&threads_mgr.atomic_barrier_count, __ATOMIC_ACQUIRE) == 0)
        return;

    if (__atomic_fetch_sub(&threads_mgr.atomic_barrier_count, 1, __ATOMIC_RELAXED) == 1)
        ;
    else
        while (__atomic_load_n(&threads_mgr.atomic_barrier_count, __ATOMIC_ACQUIRE) != 0)
            ;
}

unsigned int thread_init(void) {
    unsigned int i;

    // find the vcore struct to use
    for (i = 0; i < ctx.cores; i++) {
        if (pthread_equal(pthread_self(), GET_THREAD_ID(i)) != 0)
            break;
    }

    return i;
}

static void threads_mgr_sleep_core(Halt_Cond *halt_cond) {
    pthread_mutex_lock_(&halt_cond->mutex);
    while (halt_cond->halted) {
#ifdef DEBUG_THREAD_MGR
        int idx = thread_init();
        printf("CORE %d HALTED\n", idx);
#endif
        pthread_cond_wait(&halt_cond->cond, &halt_cond->mutex);
    }
    pthread_mutex_unlock_(&halt_cond->mutex);
}

static void *core_thread_fun(void *args) {

    unsigned int core_idx = thread_init();
    VCore *core = &GET_CORE(core_idx);
    Halt_Cond *halt_cond = &GET_HALT(core_idx);
    core->core_idx = core_idx;

    while (1) {
        // if USER mode and debug is deactivated, just run without using the mutexes
        if (ctx.debug || SUPERVISOR_IS_SET) {
            // core 0 is by default set to not halted so it will not stop on this
            threads_mgr_sleep_core(halt_cond);
        }

        if (ctx.debug) {
            // start the threads at the same time after a run all
            __BARRIER_COUNT_WAIT__;

            vcore_step(core);
            // Signal step (in order to unlock the debug thread that is waiting to tell GDB that
            // the step command finished executing)
            // if the core is just running and not doing a step this operation is useless
            __RESET_COND__(&halt_cond->atomic_step);

            // Used from the debug thread to have a temporary "stop all" in order
            // to access the "halted" condition wait and set them according to the
            // thread to run or stop.
            // after that the debug thread releases all the threads that just go to sleep
            // on the pthread_cond_wait if they are set with "halted" to true
            __WAIT_IF_SET__(&threads_mgr.atomic_stop_all);
        } else {
            // we go directly here only in USER mode and without debugging
            // just 1 core that runs directly
            vcore_run(core);
        }
    }

    return NULL;
}

void threads_mgr_init(void) {
    int ret;

    // TODO: make them aligned to 64 bytes in order to avoid
    // false sharing
    threads_mgr.threads_cores = malloc(sizeof(Thread) * ctx.cores);
    if (threads_mgr.threads_cores == NULL)
        SV32_CRASH("OOM");

    threads_mgr.atomic_stop_all = false;
    threads_mgr.atomic_barrier_count = 0;
    threads_mgr.halt_cond = NULL;

    for (unsigned int i = 0; i < ctx.cores; i++)
        memset(&GET_CORE(i), 0, sizeof(VCore));

    // if SUPERVISOR IS SET we want to allocate the debug structures
    // anyway, because we can use them to implement the SBI HSM extension
    if (!ctx.debug && !SUPERVISOR_IS_SET)
        return;

    pthread_mutex_init(&threads_mgr.halt_all_n_run_mutex, NULL);

    // set debugging structs
    threads_mgr.halt_cond = malloc(sizeof(Halt_Cond) * ctx.cores);
    if (threads_mgr.halt_cond == NULL)
        SV32_CRASH("OOM");

    for (unsigned int i = 0; i < ctx.cores; i++) {
        GET_HALT(i).halted = true;
        GET_HALT(i).atomic_step = false;
        pthread_mutex_init(&GET_HALT(i).mutex, NULL);
        ret = pthread_cond_init(&GET_HALT(i).cond, NULL);
        if (ret != 0)
            SV32_CRASH("PTHREAD COND INIT");
    }

    // In SUPERVISOR configuration CORE 0 just runs anyway
    // during debug instead all threads are stopped as default
    if (!ctx.debug)
        GET_HALT(0).halted = false;
}

bool threads_mgr_is_halted(unsigned int core_idx) {
    assert(core_idx < ctx.cores);

    int ret;
    bool status = false;

    ret = pthread_mutex_trylock(&GET_HALT(core_idx).mutex);
    if (ret == EBUSY) {
        sched_yield();
    } else {
        status = GET_HALT(core_idx).halted;
        pthread_mutex_unlock_(&GET_HALT(core_idx).mutex);
    }

    return status;
}

void threads_mgr_run(void) {
    int ret;
    if (ctx.debug) {
        // create debug thread
        ret = pthread_create(&threads_mgr.debug_thread, NULL, debug_thread_fun, NULL);
        if (ret != 0)
            SV32_CRASH("PTHREAD CREATE");
    }

    // from 1 because the main thread is already core 0
    for (unsigned int i = 1; i < ctx.cores; i++) {
        ret = pthread_create(&GET_THREAD_ID(i), NULL, core_thread_fun, NULL);
        if (ret != 0)
            SV32_CRASH("PTHREAD CREATE");
    }

    GET_THREAD_ID(0) = pthread_self();
    core_thread_fun(NULL);
}

void threads_mgr_halt_core(unsigned int core_idx) {
    assert(core_idx < ctx.cores);

    pthread_mutex_lock(&GET_HALT(core_idx).mutex);
    GET_HALT(core_idx).halted = true;
    pthread_mutex_unlock(&GET_HALT(core_idx).mutex);
}

void threads_mgr_halt_all(void) {
    // we're serializing the halt_all and run/run_all
    // making the process of stopping the single cores (unrelated mutexes and conditions)
    // into a single uninterruptable operation

    pthread_mutex_lock_(&threads_mgr.halt_all_n_run_mutex);

    // make cores wait here
    __SET_COND__(&threads_mgr.atomic_stop_all);

    // activate all the "halted" variables in order to
    // put the cores to sleep when resetting the atomic_stop_all

    for (unsigned int i = 0; i < ctx.cores; i++) {
        threads_mgr_halt_core(i);
    }

    // relase them
    // and put them to sleep on the pthread_cond
    __RESET_COND__(&threads_mgr.atomic_stop_all);

#ifdef DEBUG_THREAD_MGR
    printf("CORES HALTED, notifying gdb\n");
#endif

    pthread_mutex_unlock_(&threads_mgr.halt_all_n_run_mutex);
}

static void threads_mgr_run_core_internal(unsigned int core_idx) {
    pthread_mutex_lock_(&GET_HALT(core_idx).mutex);
    GET_HALT(core_idx).halted = false;
    pthread_cond_signal(&GET_HALT(core_idx).cond);
    pthread_mutex_unlock_(&GET_HALT(core_idx).mutex);
}

void threads_mgr_run_core(unsigned int core_idx) {
    assert(core_idx < ctx.cores);

    int ret;

    // when racing with an halt all operation (mutex already locked)
    // just "abort" the "run" operation and return
    // here we're enforcing an "ALWAYS STOP" semantic in order to completely avoid
    // so a core can be run only if we're not racing against an halt operation
    ret = pthread_mutex_trylock(&threads_mgr.halt_all_n_run_mutex);
    if (ret == EBUSY) {
        return;
    }
    threads_mgr_run_core_internal(core_idx);

    pthread_mutex_unlock_(&threads_mgr.halt_all_n_run_mutex);
}

void threads_mgr_run_all(void) {
    int ret;

    // when racing with an halt all operation (mutex already locked)
    // just "abort" the "run" operation and return
    // here we're enforcing an "ALWAYS STOP" semantic in order to completely avoid
    // so a core can be run only if we're not racing against an halt operation
    ret = pthread_mutex_trylock(&threads_mgr.halt_all_n_run_mutex);
    if (ret == EBUSY) {
        return;
    }

    // start cores at the same time
    __RESET_BARRIER_COUNT__;

    for (unsigned int i = 0; i < ctx.cores; i++) {
        threads_mgr_run_core_internal(i);
    }

    pthread_mutex_unlock_(&threads_mgr.halt_all_n_run_mutex);
}

void threads_mgr_step_core(unsigned int core_idx) {
    assert(core_idx < ctx.cores);
    assert(ctx.debug && "This function need to be used only during debugging");

    // activate the step
    __SET_COND__(&GET_HALT(core_idx).atomic_step);

    // prepare to stop the core after stepping
    __SET_COND__(&threads_mgr.atomic_stop_all);

    // release the core (using the internal function that doesn't lock on the halt_all_n_run mutex
    // because we're assuming that the step is used only when debugging and so
    // all the cores are already halted at this point, so it's impossible to have a race
    // between stepping and halting of any core)
    threads_mgr_run_core_internal(core_idx);

    // wait to have the ok about the "step" from the core
    __WAIT_IF_SET__(&GET_HALT(core_idx).atomic_step);

#ifdef DEBUG_THREAD_MGR
    printf("CORE %d FINISHED STEPPING, notifying gdb\n", core_idx);
#endif

    // now the thread should be waiting on atomic_stop_all

    // prepare for halt
	
	threads_mgr_halt_core(core_idx);

    // release the thread that will halt on "halted" condition
    __RESET_COND__(&threads_mgr.atomic_stop_all);
}
