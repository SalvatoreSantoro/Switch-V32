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

#define __WAIT_STOP_ALL__                                                                                              \
    do {                                                                                                               \
        while (__atomic_load_n(&threads_mgr.atomic_stop_all, __ATOMIC_ACQUIRE)) {                                      \
        }                                                                                                              \
    } while (0)

#define __WAIT_STEP__(atomic_step_ref)                                                                                 \
    do {                                                                                                               \
        while (__atomic_load_n(atomic_step_ref, __ATOMIC_ACQUIRE)) {                                                   \
        }                                                                                                              \
    } while (0)

#define __STOP_ALL__ __atomic_store_n(&threads_mgr.atomic_stop_all, true, __ATOMIC_RELEASE)

#define __SET_STEP__(atomic_step_ref) __atomic_store_n(atomic_step_ref, true, __ATOMIC_RELEASE)

#define __SIGNAL_STOP_ALL__ __atomic_clear(&threads_mgr.atomic_stop_all, __ATOMIC_RELEASE)

#define __SIGNAL_STEP__(atomic_step_ref) __atomic_clear(atomic_step_ref, __ATOMIC_RELEASE)

#define __BARRIER_COUNT_WAIT__ barrier_count_wait()

#define __RESET_BARRIER_COUNT__ __atomic_store_n(&threads_mgr.atomic_barrier_count, ctx.cores, __ATOMIC_RELEASE)

void barrier_count_wait(void) {
    if (__atomic_load_n(&threads_mgr.atomic_barrier_count, __ATOMIC_ACQUIRE) == 0)
        return;

    if (__atomic_fetch_sub(&threads_mgr.atomic_barrier_count, 1, __ATOMIC_RELAXED) == 1)
        ;
    else
        while (__atomic_load_n(&threads_mgr.atomic_barrier_count, __ATOMIC_ACQUIRE) != 0)
            ;
}

int thread_init() {
    int i;

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

    int core_idx = thread_init();
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
            __SIGNAL_STEP__(&halt_cond->atomic_step);

            // Used from the debug thread to have a temporary "stop all" in order
            // to access the "halted" condition wait and set them according to the
            // thread to run or stop
            // aftet that the debug thread releases all the threads that just go to sleep
            // on the pthread_cond_wait if they are set with "halted" to true
            __WAIT_STOP_ALL__;
        } else {
            vcore_run(core);
        }
    }

    return NULL;
}

void threads_mgr_init() {

    int ret;

    // TODO: make them aligned to 64 bytes in order to avoid
    // false sharing
    threads_mgr.threads_cores = malloc(sizeof(Thread) * ctx.cores);
    if (threads_mgr.threads_cores == NULL)
        SV32_CRASH("OOM");

    threads_mgr.atomic_stop_all = false;
    threads_mgr.atomic_barrier_count = 0;
    threads_mgr.halt_cond = NULL;

    for (int i = 0; i < ctx.cores; i++)
        memset(&GET_CORE(i), 0, sizeof(VCore));

    // if SUPERVISOR IS SET we want to allocate the debug structures
    // anyway, because we can use them to implement the SBI HSM extension
    if (!ctx.debug && !SUPERVISOR_IS_SET)
        return;

    // set debugging structs
    threads_mgr.halt_cond = malloc(sizeof(Halt_Cond) * ctx.cores);
    if (threads_mgr.halt_cond == NULL)
        SV32_CRASH("OOM");

    for (int i = 0; i < ctx.cores; i++) {
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

bool threads_mgr_is_halted(int core_idx) {
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

void threads_mgr_run() {
    int ret;
    if (ctx.debug) {
        // create debug thread
        ret = pthread_create(&threads_mgr.debug_thread, NULL, debug_thread_fun, NULL);
        if (ret != 0)
            SV32_CRASH("PTHREAD CREATE");
    }

    // from 1 because the main thread is already core 0
    for (int i = 1; i < ctx.cores; i++) {
        ret = pthread_create(&GET_THREAD_ID(i), NULL, core_thread_fun, NULL);
        if (ret != 0)
            SV32_CRASH("PTHREAD CREATE");
    }

    GET_THREAD_ID(0) = pthread_self();
    core_thread_fun(NULL);
}

void threads_mgr_halt_core(int core_idx) {
    assert(core_idx < ctx.cores);

    pthread_mutex_lock(&GET_HALT(core_idx).mutex);
    GET_HALT(core_idx).halted = true;
    pthread_mutex_unlock(&GET_HALT(core_idx).mutex);
}

void threads_mgr_halt_all() {

    // make threads wait here
    __STOP_ALL__;

    // activate all the "halted" variables in order to
    // put them to sleep when releasing the STOP_ALL

    for (int i = 0; i < ctx.cores; i++) {
        threads_mgr_halt_core(i);
    }

    // put them to sleep
    __SIGNAL_STOP_ALL__;

#ifdef DEBUG_THREAD_MGR
    printf("CORES HALTED, notifying gdb\n");
#endif
}

void threads_mgr_run_core(int core_idx) {
    assert(core_idx < ctx.cores);

    pthread_mutex_lock_(&GET_HALT(core_idx).mutex);
    GET_HALT(core_idx).halted = false;
    pthread_cond_signal(&GET_HALT(core_idx).cond);
    pthread_mutex_unlock_(&GET_HALT(core_idx).mutex);
}

void threads_mgr_run_all() {

    // start them at the same time
    __RESET_BARRIER_COUNT__;

	// if placing the cores to sleep, wait here untill all the cores are sleeping
	// in this way when trying to run_all we can assume that an eventual "stop_all"
	// isn't racing with this run_all, generating a state in which some cores are running
	// and some are stopped
	// run_all and halt_all should never race because when running we're running all the cores
	// togheter (so no-one can issue an halt all) and when halting we can't run anything
	// because until the cores are all halted control isn't returned to GDB
	// so this check should be redundant
    __WAIT_STOP_ALL__;

    for (int i = 0; i < ctx.cores; i++) {
        threads_mgr_run_core(i);
    }
}

void threads_mgr_step_core(int core_idx) {

    assert(core_idx < ctx.cores);
    // activate the step
    __SET_STEP__(&GET_HALT(core_idx).atomic_step);

    // prepare to stop the core after stepping
    __STOP_ALL__;

    // release the core
    threads_mgr_run_core(core_idx);

    // wait to have the ok about the "step" from the core

    __WAIT_STEP__(&GET_HALT(core_idx).atomic_step);

#ifdef DEBUG_THREAD_MGR
    printf("CORE %d FINISHED STEPPING, notifying gdb\n", core_idx);
#endif

    // now the thread should be waiting on atomic_stop_all

    // prepare the halt

    pthread_mutex_lock_(&GET_HALT(core_idx).mutex);
    GET_HALT(core_idx).halted = true;
    pthread_mutex_unlock_(&GET_HALT(core_idx).mutex);

    // release the thread that will halt on "halted" condition
    __SIGNAL_STOP_ALL__;
}
