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

#define BARRIER_COUNT_WAIT__ barrier_count_wait()

#define RESET_BARRIER_COUNT__                                                                                          \
    do {                                                                                                               \
        __atomic_store_n(&threads_mgr.atomic_barrier_count, ctx.cores, __ATOMIC_RELEASE);                              \
    } while (0)

// wait until barrier reaches 0, if already 0 just return

static void barrier_count_wait(void) {
    // fast path: if already 0, no need to wait
    if (__atomic_load_n(&threads_mgr.atomic_barrier_count, __ATOMIC_ACQUIRE) == 0)
        return;

    // decrement counter
    unsigned int prev = __atomic_fetch_sub(&threads_mgr.atomic_barrier_count, 1, __ATOMIC_RELAXED);

    if (prev == 1) {
        // This thread “released” the barrier
    } else {
        // wait until counter reaches 0
        while (__atomic_load_n(&threads_mgr.atomic_barrier_count, __ATOMIC_ACQUIRE) != 0) {
        }
    }
}

bool threads_mgr_are_halted(void) {
    cthread_state state;

    for (unsigned int i = 0; i < ctx.cores; i++) {
        state = cthread_get_state(&threads_mgr.cthreads[i]);
        if (state != STATE_HALTED) {
            return false;
        }
    }
    return true;
}

void threads_mgr_init(void) {
    // TODO: make them aligned to 64 bytes in order to avoid
    // false sharing
    threads_mgr.cthreads = malloc(sizeof(Cthread) * ctx.cores);
    if (threads_mgr.cthreads == NULL)
        SV32_CRASH("OOM");

    threads_mgr.atomic_barrier_count = 0;

    // core 0 will be run when using a signal start
    cthread_init(&threads_mgr.cthreads[0], STATE_STARTED);

    // other cores need to be signaled to halted before starting
    // so that HSM states and debugging states are coherent
    for (unsigned int i = 1; i < ctx.cores; i++)
        cthread_init(&threads_mgr.cthreads[i], STATE_STOPPED);

    for (unsigned int i = 0; i < ctx.cores; i++) {
        cthread_run(&threads_mgr.cthreads[i]);
    }
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
    bool not_found = true;

    for (unsigned int i = 0; i < ctx.cores; i++) {
        // don't synchronize with yourself (deadlock)
        if (not_found && !!pthread_equal(pthread_self(), threads_mgr.cthreads[i].thread_id)) {
            cthread_signal_halt(&threads_mgr.cthreads[i], false);
            not_found = false;
        } else {
            cthread_signal_halt(&threads_mgr.cthreads[i], true);
        }
    }
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
}
