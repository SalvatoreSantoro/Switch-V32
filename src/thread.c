#include "thread.h"
#include "cpu.h"
#include "macros.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>

#define SV32_CRASH(msg)                                                                                                \
    do {                                                                                                               \
        fprintf(stderr, "[SV32_CRASH] %s:%d: %s\n", __FILE__, __LINE__, (msg));                                        \
        fflush(stderr);                                                                                                \
    } while (0)

static void thread_enter_state(Thread *thread, Thread_State entering_state) {
    // set state "entering_state" to "ack" the
    // state transition caused by "signal"
    thread->state = entering_state;
    pthread_cond_signal_(&thread->cond_state);
}

static void thread_wait_on_sign(Thread *thread, Thread_Signal signal) {
    while (thread->signal == signal) {
        pthread_cond_wait_(&thread->cond_signal, &thread->mutex);
    }
}

// if synch = true wait for response
void thread_signal_start(Thread *thread, bool synch) {
    pthread_mutex_lock_(&thread->mutex);

    Thread_State state = thread->state;
    // the signal is processed only when called while the core
    // is "STATE_HALTED" or "STATE_SUSPENDED"
    if ((state != STATE_HALTED) && (state != STATE_SUSPENDED)) {
        pthread_mutex_unlock_(&thread->mutex);
        return;
    }

    __atomic_store_n(&thread->core.atomic_stop, 0, __ATOMIC_RELEASE);

    thread->signal = START_S;
    pthread_cond_signal_(&thread->cond_signal);

    if (synch) {
        while (thread->state != STATE_STARTED) {
            pthread_cond_wait_(&thread->cond_state, &thread->mutex);
        }
    } else {
        // check if starting from SUSPENDED or from HALTED
        thread->state = thread->state == STATE_HALTED ? STATE_START_PENDING : STATE_RESUME_PENDING;
    }

    pthread_mutex_unlock_(&thread->mutex);
}

void thread_signal_start_debug(Thread *thread) {
    pthread_mutex_lock_(&thread->mutex);

    Thread_State state = thread->state;
    // the signal is processed only when called while the core
    // is "STATE_HALTED"
    if (state != STATE_STOPPED) {
        pthread_mutex_unlock_(&thread->mutex);
        return;
    }

    thread->signal = START_DEBUG_S;
    pthread_cond_signal_(&thread->cond_signal);

    while (thread->state != STATE_HALTED) {
        pthread_cond_wait_(&thread->cond_state, &thread->mutex);
    }

    pthread_mutex_unlock_(&thread->mutex);
}

// be careful with synch=true, if a core is halting itself
// making this synchronous will deadlock the core
void thread_signal_halt(Thread *thread, bool synch) {
    pthread_mutex_lock_(&thread->mutex);

    Thread_State state = thread->state;
    // the signal is processed only when called while the core
    // is "STATE_STARTED" or "STATE_STEPPING"
    if ((state != STATE_STARTED) && (state != STATE_STEPPING)) {
        pthread_mutex_unlock_(&thread->mutex);
        return;
    }

    __atomic_store_n(&thread->core.atomic_stop, 1, __ATOMIC_RELEASE);

    thread->signal = HALT_S;
    pthread_cond_signal_(&thread->cond_signal);

    while (synch && (thread->state != STATE_HALTED)) {
        pthread_cond_wait_(&thread->cond_state, &thread->mutex);
    }

    pthread_mutex_unlock_(&thread->mutex);
}

void thread_signal_stop(Thread *thread, bool synch) {
    pthread_mutex_lock_(&thread->mutex);

    Thread_State state = thread->state;
    // the signal is processed only when called while the core
    // is "STATE_STARTED" or "STATE_HALTED"
    if ((state != STATE_STARTED) && (state != STATE_HALTED)) {
        pthread_mutex_unlock_(&thread->mutex);
        return;
    }

    __atomic_store_n(&thread->core.atomic_stop, 1, __ATOMIC_RELEASE);

    thread->signal = STOP_S;
    pthread_cond_signal_(&thread->cond_signal);

    if (synch) {
        while (thread->state != STATE_STOPPED) {
            pthread_cond_wait_(&thread->cond_state, &thread->mutex);
        }
    } else {
        thread->state = STATE_STOP_PENDING;
    }

    pthread_mutex_unlock_(&thread->mutex);
}

void thread_signal_suspend(Thread *thread, bool synch) {
    pthread_mutex_lock_(&thread->mutex);

    // the signal is processed only when called while the core
    // is "STATE_STARTED"
    if (thread->state != STATE_STARTED) {
        pthread_mutex_unlock_(&thread->mutex);
        return;
    }

    __atomic_store_n(&thread->core.atomic_stop, 1, __ATOMIC_RELEASE);

    thread->signal = SUSPEND_S;
    pthread_cond_signal_(&thread->cond_signal);

    if (synch) {
        while (thread->state != STATE_SUSPENDED) {
            pthread_cond_wait_(&thread->cond_state, &thread->mutex);
        }
    } else {
        thread->state = STATE_SUSPEND_PENDING;
    }

    pthread_mutex_unlock_(&thread->mutex);
}

// if synch = true wait for response
// not using
// __atomic_store_n(&thread->core.atomic_stop, 1, __ATOMIC_RELEASE);
// because assuming that we get here only from an halted state (so atomic_stop was already set)
void thread_signal_step(Thread *thread) {
    pthread_mutex_lock_(&thread->mutex);

    if (thread->state != STATE_HALTED) {
        pthread_mutex_unlock_(&thread->mutex);
        return;
    }

    thread->signal = STEP_S;
    pthread_cond_signal_(&thread->cond_signal);

    // wait for stepping
    while (thread->state != STATE_STEPPING) {
        pthread_cond_wait_(&thread->cond_state, &thread->mutex);
    }

    // assume that while executing the core could take a breakpoint and stop itself
    // so always check if the core is already stopped at this point
    if (thread->signal != HALT_S) {
        thread->signal = HALT_S;
        pthread_cond_signal_(&thread->cond_signal);
    }

    // wait for completition
    while (thread->state != STATE_HALTED) {
        pthread_cond_wait_(&thread->cond_state, &thread->mutex);
    }

    pthread_mutex_unlock_(&thread->mutex);
}

void thread_init(Thread *thread, Thread_State state) {
    switch (state) {
    case STATE_START_PENDING:
    case STATE_STOP_PENDING:
    case STATE_SUSPEND_PENDING:
    case STATE_RESUME_PENDING:
        SV32_CRASH("WRONG THREAD STATE INITIALIZATION");
    default:
        break;
    }

    vcore_reset(&thread->core);
    int ret;

    thread->state = state;
    thread->signal = (Thread_Signal) state;
    pthread_mutex_init(&thread->mutex, NULL);
    ret = pthread_cond_init(&thread->cond_signal, NULL);
    ret = pthread_cond_init(&thread->cond_signal, NULL);
    if (ret != 0)
        SV32_CRASH("PTHREAD COND INIT");
}

static void thread_fsm(Thread *thread) {
    // state transition
    pthread_mutex_lock_(&thread->mutex);
    switch (thread->signal) {
    case STOP_S:
        thread_enter_state(thread, STATE_STOPPED);
        thread_wait_on_sign(thread, STOP_S);
        break;
    case START_DEBUG_S:
        thread_enter_state(thread, STATE_HALTED);
		printf("SGS 1\n");
        thread_wait_on_sign(thread, START_DEBUG_S);
        break;
    case HALT_S:
        thread_enter_state(thread, STATE_HALTED);
		printf("SGS 2\n");
        thread_wait_on_sign(thread, HALT_S);
        break;
    case START_S:
        thread_enter_state(thread, STATE_STARTED);
        break;
    case SUSPEND_S:
        thread_enter_state(thread, STATE_SUSPENDED);
        thread_wait_on_sign(thread, SUSPEND_S);
        break;
    case STEP_S:
        thread_enter_state(thread, STATE_STEPPING);
        thread_wait_on_sign(thread, STEP_S);
        break;
    default:
        printf("%d\n", thread->signal);
        SV32_CRASH("Unexpected core SIGNAL\n");
    }
    Thread_State state = thread->state;
    pthread_mutex_unlock_(&thread->mutex);

    // run core
    switch (state) {
    case STATE_STARTED:
    case STATE_STEPPING:
        vcore_run(&thread->core);
        break;
    default:
        break;
    }
}

static void *thread_fun(void *args) {
    Thread *thread = (Thread *) args;

    while (1) {
        thread_fsm(thread);
    }
    return NULL;
}

Thread_State thread_get_state(Thread *thread) {
    Thread_State state;

    pthread_mutex_lock_(&thread->mutex);
    state = thread->state;
    pthread_mutex_unlock_(&thread->mutex);

    return state;
}

void thread_run(Thread *thread) {
    int ret;
    pthread_attr_t attr;

    ret = pthread_attr_init(&attr);
    if (ret != 0)
        SV32_CRASH("pthread_attr_init failed");

    ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (ret != 0)
        SV32_CRASH("pthread_attr_setdetachstate failed");

    ret = pthread_create(&thread->thread_id, &attr, thread_fun, thread);
    if (ret != 0)
        SV32_CRASH("pthread_create failed");

    pthread_attr_destroy(&attr);
}
