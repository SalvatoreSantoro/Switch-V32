#include "threads_mgr2.h"
#include "args.h"
#include "cpu.h"
#include "cthread.h"
#include "debug.h"
#include "defs.h"
#include "macros.h"
#include <assert.h>
#include <errno.h> // IWYU pragma: export
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#define pthread_cond_wait_(cond_ref, mutex_ref)                                                                        \
    do {                                                                                                               \
        if (pthread_cond_wait(cond_ref, mutex_ref) != 0)                                                               \
            SV32_CRASH("COND WAIT FAILED");                                                                            \
    } while (0)

#define pthread_cond_signal_(cond_ref)                                                                                 \
    do {                                                                                                               \
        if (pthread_cond_signal(cond_ref) != 0)                                                                        \
            SV32_CRASH("SIGNAL FAILED");                                                                               \
    } while (0)

bool threads_mgr_are_halted(void) {
    cthread_state state;

    for (unsigned int i = 0; i < ctx.cores; i++) {
        state = cthread_get_state(&threads_mgr.cthreads[i]);
        // printf("STATE OF %d was: %d old signal was: %d\n", i, state, threads_mgr.cthreads[i].old_signal);
        if (state != STATE_HALTED) {
            return false;
        }
    }
    return true;
}

void threads_mgr_init(VCore_Init *core0_init) {
    // TODO: make them aligned to 64 bytes in order to avoid
    // false sharing
    VCore_Init cores_init[ctx.cores];

    for (unsigned int i = 0; i < ctx.cores; i++) {
        memset(&cores_init[i], 0, sizeof(cores_init[i]));
        cores_init[i].id = i;
    }

    threads_mgr.cthreads = malloc(sizeof(Cthread) * ctx.cores);
    if (threads_mgr.cthreads == NULL)
        SV32_CRASH("OOM");

    pthread_mutex_init(&threads_mgr.halt_all_n_run_mutex, NULL);

    // core 0 will be run when using a signal start
    cthread_init(&threads_mgr.cthreads[0], STATE_STARTED, core0_init);

    // other cores need to be signaled to halted before starting
    // so that HSM states and debugging states are coherent
    for (unsigned int i = 1; i < ctx.cores; i++)
        cthread_init(&threads_mgr.cthreads[i], STATE_STOPPED, &cores_init[i]);

    for (unsigned int i = 0; i < ctx.cores; i++)
        cthread_run(&threads_mgr.cthreads[i]);
}

void threads_mgr_step_core(unsigned int core_idx) {
    assert(ctx.debug && "This function need to be used only during debugging");

    cthread_state state;

    state = cthread_get_state(&threads_mgr.cthreads[core_idx]);
    if (state != STATE_HALTED) {
        SV32_CRASH("CONTINUE CORE BROKEN INVARIANT.");
    }

    cthread_signal_step(&threads_mgr.cthreads[core_idx]);
}

void threads_mgr_halt_cores(void) {

    // we're serializing the halt_all and run/run_all
    // making the process of stopping the single cores (unrelated mutexes and conditions)
    // into a single uninterruptable operation
    pthread_mutex_lock_(&threads_mgr.halt_all_n_run_mutex);

    for (unsigned int i = 0; i < ctx.cores; i++)
        cthread_signal_halt(&threads_mgr.cthreads[i]);
    
    pthread_mutex_unlock_(&threads_mgr.halt_all_n_run_mutex);
}

void threads_mgr_continue_core(unsigned int core_idx) {
    assert(ctx.debug && "This function need to be used only during debugging");

    cthread_state state;
    state = cthread_get_state(&threads_mgr.cthreads[core_idx]);
    if (state != STATE_HALTED) {
        SV32_CRASH("CONTINUE CORE BROKEN INVARIANT.");
    }
    cthread_signal_continue(&threads_mgr.cthreads[core_idx]);
}

void threads_mgr_continue_cores(void) {
    cthread_state state;
    int ret;

    // when racing with an halt all operation (mutex already locked)
    // just "abort" the "run" operation and return
    // here we're enforcing an "ALWAYS STOP" semantic
    // so a core can be run only if we're not racing against an halt all operation
    ret = pthread_mutex_trylock(&threads_mgr.halt_all_n_run_mutex);
    if (ret == EBUSY)
        return;

    // Should continue all only if they were all stopped
    for (unsigned int i = 0; i < ctx.cores; i++) {
        state = cthread_get_state(&threads_mgr.cthreads[i]);
        if (state != STATE_HALTED) {
            SV32_CRASH("CONTINUE ALL BROKEN INVARIANT.");
        }
    }

    // continue all
    for (unsigned int i = 0; i < ctx.cores; i++) {
        cthread_signal_continue(&threads_mgr.cthreads[i]);
    }

    pthread_mutex_unlock_(&threads_mgr.halt_all_n_run_mutex);
}
