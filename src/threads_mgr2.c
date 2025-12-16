#include "threads_mgr2.h"
#include "args.h"
#include "cpu.h"
#include "debug.h"
#include "defs.h"
#include "macros.h"
#include "thread.h"
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

static void threads_mgr_check_stop(unsigned int core_idx) {
    while (GET_SIGNAL(core_idx) == STOP_S) {
        if (GET_STATE(core_idx) != STATE_STOPPED) {
            SET_STATE(core_idx, STATE_STOPPED);
            pthread_cond_signal_(&GET_STATE_COND(core_idx));
        }
        pthread_cond_wait_(&GET_SIGN_COND(core_idx), &GET_MUTEX(core_idx));
    }
}

static void threads_mgr_check_halt(unsigned int core_idx) {
    while (GET_SIGNAL(core_idx) == HALT_S) {
        if (GET_STATE(core_idx) != STATE_HALTED) {
            SET_STATE(core_idx, STATE_HALTED);
            pthread_cond_signal_(&GET_STATE_COND(core_idx));
        }
        pthread_cond_wait_(&GET_SIGN_COND(core_idx), &GET_MUTEX(core_idx));
    }
}

static void threads_mgr_check_run(unsigned int core_idx) {
    if (GET_SIGNAL(core_idx) == RUN_S) {
        // Signal that the core is running (in case of run or step)
        if (GET_STATE(core_idx) != STATE_RUNNING) {
            SET_STATE(core_idx, STATE_RUNNING);
            pthread_cond_signal_(&GET_STATE_COND(core_idx));
        }
    }
}

static void threads_mgr_check_step(unsigned int core_idx) {
    if (GET_SIGNAL(core_idx) == STEP_S) {
        if (GET_STATE(core_idx) != STATE_STEPPING) {
            SET_STATE(core_idx, STATE_STEPPING);
            pthread_cond_signal_(&GET_STATE_COND(core_idx));
        }
        // wait for setup of the stop in order to block right after doing the step
        while (GET_SIGNAL(core_idx) != HALT_S) {
            pthread_cond_wait_(&GET_SIGN_COND(core_idx), &GET_MUTEX(core_idx));
        }
    }
}

static void threads_mgr_core_state_check(unsigned int core_idx) {
    pthread_mutex_lock_(&GET_MUTEX(core_idx));

    switch (GET_STATE(core_idx)) {
    // off
    case STATE_STOPPED:
        threads_mgr_check_stop(core_idx);
        break;
    case STATE_HALTED:
        // halt
        threads_mgr_check_halt(core_idx);
        break;
    case STATE_RUNNING:
        // run
        threads_mgr_check_run(core_idx);
        break;
    case STATE_STEPPING:
        // step
        threads_mgr_check_step(core_idx);
        break;
    case STATE_SUSPENDED:
        break;
    default:
        SV32_CRASH("Unexpected core state\n");
    }
    pthread_mutex_unlock_(&GET_MUTEX(core_idx));
}


static void *core_thread_fun(void *args) {
    Thread_Args *casted_args = (Thread_Args *) args;
    VCore *core = &casted_args->core;
    unsigned int core_idx = core->core_idx;

    while (1) {
        threads_mgr_core_state_check(core_idx);
        // start the threads at the same time after a run all
        BARRIER_COUNT_WAIT__;
        vcore_run(core);
    }
    return NULL;
}

void threads_mgr_init(void) {
    // TODO: make them aligned to 64 bytes in order to avoid
    // false sharing
    threads_mgr.threads = malloc(sizeof(Thread) * ctx.cores);
    if (threads_mgr.threads_args == NULL)
        SV32_CRASH("OOM");

    threads_mgr.atomic_barrier_count = 0;
    pthread_mutex_init(&threads_mgr.halt_all_n_run_mutex, NULL);

	// core 0 will be run when using a signal start
    thread_init(threads_mgr.threads[i], STATE_HALTED, HALT_S);

	// other cores need to be signaled to halted before starting
	// so that HSM states and debugging states are coherent
    for (unsigned int i = 1; i < ctx.cores; i++)
        thread_init(threads_mgr.threads[i], STATE_STOPPED, STOP_S);
}

bool threads_mgr_is_halted(unsigned int core_idx, bool synch) {
    assert(core_idx < ctx.cores);

    Core_State status = STATE_RUNNING;

    if (synch) {
        pthread_mutex_lock_(&GET_MUTEX(core_idx));
    } else {
        int ret;
        ret = pthread_mutex_trylock(&GET_MUTEX(core_idx));
        if (ret == EBUSY) {
            return false;
        }
    }

    status = GET_STATE(core_idx);
    pthread_mutex_unlock_(&GET_MUTEX(core_idx));

// when mixing SBI_HSM with GDB debugger:
// there is the possibility that a core results halted (because it's powered off)
// so after a continue all, if gdb find that a core is halted, it assumes that
// every core is halted (the only way to stop a core after a "continue all" is through a breakpoint that stops
// everything)
// but in reality if some cores are "POWERED_OFF" they will not run when gdb sends a "continue all"
// (in a certain sense HSM has a priority over RUNNING/HALTED states used by gdb), so they
// will result already halted if gdb run them, even if in reality they didn't even started
// so in general before reporting to gdb that a core is halted, we just enforce an halt all
#ifdef SUPERVISOR
    if (status == STATE_HALTED) {
        threads_mgr_halt_all();
        return true;
    }
#endif

    return status == STATE_HALTED ? true : false;
}

void threads_mgr_run(void) {
    int ret;

    // from 1 because the main thread is already core 0
    for (unsigned int i = 1; i < ctx.cores; i++) {
        GET_CORE(i).core_idx = i;
        ret = pthread_create(&GET_THREAD_ID(i), NULL, core_thread_fun, &GET_ARG(i));
        if (ret != 0)
            SV32_CRASH("PTHREAD CREATE");
    }

    if (ctx.debug) {
        // create debug thread
        ret = pthread_create(&threads_mgr.debug_thread, NULL, debug_thread_fun, NULL);
        if (ret != 0)
            SV32_CRASH("PTHREAD CREATE");
    }

    GET_CORE(0).core_idx = 0;
    GET_THREAD_ID(0) = pthread_self();
    core_thread_fun(&GET_ARG(0));
}

void threads_mgr_halt_core(unsigned int core_idx) {
    assert(core_idx < ctx.cores);
    if (pthread_equal(pthread_self(), GET_THREAD_ID(core_idx)))
        threads_mgr_signal_halt(core_idx, false);
    else
        threads_mgr_signal_halt(core_idx, true);
}

void threads_mgr_halt_all(void) {
    bool not_found = true;
    // we're serializing the halt_all and run/run_all
    // making the process of stopping the single cores (unrelated mutexes and conditions)
    // into a single uninterruptable operation
    pthread_mutex_lock_(&threads_mgr.halt_all_n_run_mutex);

    // activate all the "halted" variables in order to
    // put the cores to sleep
    for (unsigned int i = 0; i < ctx.cores; i++) {
        // don't synchronize with yourself (deadlock)
        if (not_found && !!pthread_equal(pthread_self(), GET_THREAD_ID(i))) {
            threads_mgr_signal_halt(i, false);
            not_found = false;
        } else {
            threads_mgr_signal_halt(i, true);
        }
    }

    pthread_mutex_unlock_(&threads_mgr.halt_all_n_run_mutex);
}

bool threads_mgr_run_core(unsigned int core_idx) {
    assert(core_idx < ctx.cores);

    int ret;

    // when racing with an halt all operation (mutex already locked)
    // just "abort" the "run" operation and return
    // here we're enforcing an "ALWAYS STOP" semantic
    // so a core can be run only if we're not racing against an halt operation
    ret = pthread_mutex_trylock(&threads_mgr.halt_all_n_run_mutex);
    if (ret == EBUSY) {
        return false;
    }

    threads_mgr_signal_run(core_idx, true);

    pthread_mutex_unlock_(&threads_mgr.halt_all_n_run_mutex);

    return true;
}

bool threads_mgr_run_all(void) {
    int ret;

    // when racing with an halt all operation (mutex already locked)
    // just "abort" the "run" operation and return
    // here we're enforcing an "ALWAYS STOP" semantic
    // so a core can be run only if we're not racing against an halt all operation
    ret = pthread_mutex_trylock(&threads_mgr.halt_all_n_run_mutex);
    if (ret == EBUSY) {
        return false;
    }

    // start cores at the same time
    RESET_BARRIER_COUNT__;

    for (unsigned int i = 0; i < ctx.cores; i++) {
        threads_mgr_signal_run(i, true);
    }
    printf("Runned all\n");

    pthread_mutex_unlock_(&threads_mgr.halt_all_n_run_mutex);
    return true;
}

void threads_mgr_step_core(unsigned int core_idx) {
    assert(core_idx < ctx.cores);
    assert(ctx.debug && "This function need to be used only during debugging");
    // assuming cores are now halted
    assert(threads_mgr_is_halted(core_idx, true) && "Stepping core that isn't halted");

    // synch to wait for response
    threads_mgr_signal_step(core_idx);
}

Core_State threads_mgr_get_state(unsigned int core_idx) {
    Core_State status;
    pthread_mutex_lock_(&GET_MUTEX(core_idx));

    status = GET_STATE(core_idx);

    pthread_mutex_unlock_(&GET_MUTEX(core_idx));

    return status;
}

void threads_mgr_init_core(unsigned int core_idx, uint32_t opaque, uint32_t start_addr) {
    pthread_mutex_lock_(&GET_MUTEX(core_idx));

    assert(threads_mgr_is_halted(core_idx, true));

    GET_CORE(core_idx).regs[A0] = core_idx;
    GET_CORE(core_idx).regs[A1] = opaque;
    GET_CORE(core_idx).pc = start_addr;
#ifdef SUPERVISOR
    GET_CORE(core_idx).mode = SUPERVISOR;
#endif

    pthread_mutex_unlock_(&GET_MUTEX(core_idx));
}

void threads_mgr_init_cond()
