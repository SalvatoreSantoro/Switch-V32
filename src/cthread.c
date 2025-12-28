#include "cthread.h"
#include "cpu.h"
#include "macros.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>

static void cthread_enter_state(Cthread *cthread, cthread_state entering_state) {
    // set state "entering_state" to "ack" the
    // state transition caused by "signal"
    cthread->state = entering_state;
    pthread_cond_signal_(&cthread->cond_state);
}

static void cthread_wait_on_sign(Cthread *cthread, cthread_signal signal) {
    while (cthread->signal == signal) {
        // set state "entering_state" to "ack" the
        // state transition caused by "signal"
        if (cthread->state != (cthread_state) signal) {
            cthread->state = (cthread_state) signal;
            pthread_cond_signal_(&cthread->cond_state);
        }

        pthread_cond_wait_(&cthread->cond_signal, &cthread->mutex);
    }
}

// always asynchronous
void cthread_signal_start(Cthread *cthread) {
    pthread_mutex_lock_(&cthread->mutex);

    cthread_state state = cthread->state;

    // when concurring with debugging just buffer the signal for later
    // if the old state could accept the signal
    if ((state == STATE_HALTED) || (state == STATE_HALT_PENDING) || (state == STATE_CONTINUE_PENDING) ||
        (state == STATE_STEPPING)) {

        printf("SIGNALED START: CORE %d, STATE WAS: %d\n\n", cthread->core.core_idx, cthread->state);
        // assert(0);
        if ((cthread->old_signal == STOP_S) || (cthread->old_signal == SUSPEND_S))
            cthread->old_signal = START_S;
    }

    // the signal is processed only when called while the core
    // is "STATE_STOPPED" or "STATE_SUSPENDED"
    if ((state != STATE_STOPPED) && (state != STATE_SUSPENDED)) {
        printf("SIGNALED START: CORE %d, STATE WAS: %d\n\n", cthread->core.core_idx, cthread->state);
        // assert(0);

        pthread_mutex_unlock_(&cthread->mutex);
        return;
    }

    cthread->signal = START_S;
    cthread->old_signal = START_S;
    cthread->state = cthread->state == STATE_STOPPED ? STATE_START_PENDING : STATE_RESUME_PENDING;

    // now the core is effectively out of the hot loop (vcore_run)
    // so effectively reset this
    __atomic_store_n(&cthread->core.atomic_exit_loop, 0, __ATOMIC_RELEASE);

    pthread_cond_signal_(&cthread->cond_signal);

    pthread_mutex_unlock_(&cthread->mutex);
}

// always not synchronized
void cthread_signal_continue(Cthread *cthread) {
    pthread_mutex_lock_(&cthread->mutex);

    cthread_state state = cthread->state;

    // the signal is processed only when called while the core
    // is "STATE_HALTED"
    if ((state != STATE_HALTED) && (state != STATE_HALT_PENDING)) {
        printf("SIGNALED CONTINUE: CORE %d, STATE WAS: %d\n\n", cthread->core.core_idx, cthread->state);
        // assert(0);
        pthread_mutex_unlock_(&cthread->mutex);
        return;
    }

    // old states CAN'T be the debugging ones, otherwise there is a bug
    if ((cthread->old_signal == HALT_S) || (cthread->old_signal == STEP_S)) {
        pthread_mutex_unlock_(&cthread->mutex);
        SV32_CRASH("BUG IN THE STATE IMPLEMENTATION");
    }

    // restore old signal that will lead to old state before halting
    cthread->signal = cthread->old_signal;

    if (cthread->signal == START_S)
        __atomic_store_n(&cthread->core.atomic_exit_loop, 0, __ATOMIC_ACQ_REL);
    else
        __atomic_store_n(&cthread->core.atomic_exit_loop, 1, __ATOMIC_ACQ_REL);

    cthread->state = STATE_CONTINUE_PENDING;

    pthread_cond_signal_(&cthread->cond_signal);

    pthread_mutex_unlock_(&cthread->mutex);
}

void cthread_signal_halt(Cthread *cthread) {
    pthread_mutex_lock_(&cthread->mutex);

    if (cthread->state == STATE_HALTED || cthread->state == STATE_HALT_PENDING) {
        printf("SIGNALED HALT: CORE %d, STATE WAS: %d\n\n", cthread->core.core_idx, cthread->state);
        // assert(0);
        pthread_mutex_unlock_(&cthread->mutex);
        return;
    }

    
    cthread->signal = HALT_S;
    cthread->state = STATE_HALT_PENDING;

    // exit hot loop
    __atomic_store_n(&cthread->core.atomic_exit_loop, 1, __ATOMIC_ACQ_REL);

    pthread_cond_signal_(&cthread->cond_signal);

    pthread_mutex_unlock_(&cthread->mutex);
}

void cthread_signal_stop(Cthread *cthread) {
    pthread_mutex_lock_(&cthread->mutex);

    cthread_state state = cthread->state;

    // when concurring with debugging just buffer the signal for later
    if ((state == STATE_HALTED) || (state == STATE_HALT_PENDING) || (state == STATE_CONTINUE_PENDING) ||
        (state == STATE_STEPPING)) {
        printf("SIGNALED STOP: CORE %d, STATE WAS: %d\n\n", cthread->core.core_idx, cthread->state);
        // assert(0);

        if (cthread->old_signal == START_S)
            cthread->old_signal = STOP_S;
    }

    // the signal is processed only when called while the core
    // is "STATE_STARTED"
    if (state != STATE_STARTED) {
        pthread_mutex_unlock_(&cthread->mutex);
        return;
    }

    cthread->signal = STOP_S;
    cthread->old_signal = STOP_S;

    pthread_cond_signal_(&cthread->cond_signal);

    cthread->state = STATE_STOP_PENDING;

    // exit hot loop
    __atomic_store_n(&cthread->core.atomic_exit_loop, 1, __ATOMIC_RELEASE);

    pthread_mutex_unlock_(&cthread->mutex);
}

void cthread_signal_suspend(Cthread *cthread) {
    pthread_mutex_lock_(&cthread->mutex);

    // the signal is processed only when called while the core
    // is "STATE_STARTED"

    // when concurring with debugging just buffer the signal for later
    cthread_state state = cthread->state;

    if ((state == STATE_HALTED) || (state == STATE_HALT_PENDING) || (state == STATE_CONTINUE_PENDING) ||
        (state == STATE_STEPPING)) {
        printf("SIGNALED SUSPEND: CORE %d, STATE WAS: %d\n\n", cthread->core.core_idx, cthread->state);
        // assert(0);

        if (cthread->old_signal == START_S)
            cthread->old_signal = SUSPEND_S;
    }

    if (cthread->state != STATE_STARTED) {
        pthread_mutex_unlock_(&cthread->mutex);
        return;
    }

    cthread->signal = SUSPEND_S;
    cthread->old_signal = SUSPEND_S;
    pthread_cond_signal_(&cthread->cond_signal);

    cthread->state = STATE_SUSPEND_PENDING;

    // exit hot loop
    __atomic_store_n(&cthread->core.atomic_exit_loop, 1, __ATOMIC_RELEASE);

    pthread_mutex_unlock_(&cthread->mutex);
}

// if synch = true wait for response
// not using
// __atomic_store_n(&cthread->core.atomic_stop, 1, __ATOMIC_RELEASE);
// because assuming that we get here only from an halted state (so atomic_stop was already set)
void cthread_signal_step(Cthread *cthread) {
    pthread_mutex_lock_(&cthread->mutex);

    if (cthread->state != STATE_HALTED) {
        printf("SIGNALED STEP: CORE %d, STATE WAS: %d\n\n", cthread->core.core_idx, cthread->state);
        // assert(0);

        pthread_mutex_unlock_(&cthread->mutex);
        return;
    }

    cthread->signal = STEP_S;
    pthread_cond_signal_(&cthread->cond_signal);

    // exit hot loop
    __atomic_store_n(&cthread->core.atomic_exit_loop, 1, __ATOMIC_RELEASE);

    // wait for stepping
    while (cthread->state != STATE_STEPPING) {
        pthread_cond_wait_(&cthread->cond_state, &cthread->mutex);
    }

    // assume that while executing the core could take a breakpoint and stop itself
    // so always check if the core is already stopped at this point
    if (cthread->signal != HALT_S) {
        cthread->signal = HALT_S;
        pthread_cond_signal_(&cthread->cond_signal);
    }

    // wait for completition
    while (cthread->state != STATE_HALTED) {
        pthread_cond_wait_(&cthread->cond_state, &cthread->mutex);
    }

    pthread_mutex_unlock_(&cthread->mutex);
}

cthread_state cthread_get_state(Cthread *cthread) {
    cthread_state state;

    pthread_mutex_lock_(&cthread->mutex);
    state = cthread->state;
    pthread_mutex_unlock_(&cthread->mutex);

    return state;
}

cthread_state cthread_get_hsm_state(Cthread *cthread) {
    cthread_state state;

    pthread_mutex_lock_(&cthread->mutex);
    state = cthread->state;

    // take old state if the core is currently in debug mode
    // cause HSM doesn't understand debugging states
    switch (state) {
    case STATE_HALTED:
    case STATE_STEPPING:
    case STATE_HALT_PENDING:
        state = (cthread_state) cthread->old_signal;
        break;
    default:
        break;
    }

    pthread_mutex_unlock_(&cthread->mutex);

    return state;
}

static void cthread_fsm(Cthread *cthread) {
    // state transition
    pthread_mutex_lock_(&cthread->mutex);

    switch (cthread->signal) {
    case STOP_S:
        printf("Thread %d STOP_S, OLD SIGNAL: %d\n", cthread->core.core_idx, cthread->old_signal);
        cthread_wait_on_sign(cthread, STOP_S);
        break;
    case HALT_S:
        printf("Thread %d HALT_S, OLD SIGNAL: %d\n", cthread->core.core_idx, cthread->old_signal);
        cthread_wait_on_sign(cthread, HALT_S);
        break;
    case START_S:
        cthread_enter_state(cthread, STATE_STARTED);
        printf("Thread %d START_S, OLD SIGNAL: %d\n", cthread->core.core_idx, cthread->old_signal);
        pthread_mutex_unlock_(&cthread->mutex);
        vcore_run(&cthread->core);
        pthread_mutex_lock_(&cthread->mutex);
        break;
    case SUSPEND_S:
        printf("Thread %d SUSPEND_S, OLD SIGNAL: %d\n", cthread->core.core_idx, cthread->old_signal);
        cthread_wait_on_sign(cthread, SUSPEND_S);
        break;
    case STEP_S:
        printf("Thread %d STEP_S, OLD SIGNAL: %d\n", cthread->core.core_idx, cthread->old_signal);
        cthread_wait_on_sign(cthread, STEP_S);
        // only step if the core was turned on, otherwise do nothing
        if (cthread->old_signal == START_S) {
            pthread_mutex_unlock_(&cthread->mutex);
            vcore_run(&cthread->core);
            pthread_mutex_lock_(&cthread->mutex);
        }
        break;
    default:
        printf("%d\n", cthread->signal);
        SV32_CRASH("Unexpected core SIGNAL\n");
    }
    pthread_mutex_unlock_(&cthread->mutex);
}

static void *cthread_fun(void *args) {
    Cthread *cthread = (Cthread *) args;

    while (1) {
        cthread_fsm(cthread);
    }
    return NULL;
}

void cthread_init(Cthread *cthread, cthread_state state, VCore_Init *init) {
    switch (state) {
    case STATE_STARTED:
    case STATE_STOPPED:
    case STATE_SUSPENDED:
        break;
    default:
        SV32_CRASH("WRONG cthread STATE INITIALIZATION");
    }

    vcore_init(&cthread->core, init);
    int ret;

    cthread->old_signal = (cthread_signal) state;
    // start from halted and run using the signal continue
    cthread->signal = (cthread_signal) HALT_S;
    cthread->state = STATE_HALTED;
    pthread_mutex_init(&cthread->mutex, NULL);
    ret = pthread_cond_init(&cthread->cond_signal, NULL);
    ret = pthread_cond_init(&cthread->cond_state, NULL);
    if (ret != 0)
        SV32_CRASH("Pcthread COND INIT");
}

void cthread_run(Cthread *cthread) {
    int ret;
    pthread_attr_t attr;

    ret = pthread_attr_init(&attr);
    if (ret != 0)
        SV32_CRASH("pcthread_attr_init failed");

    ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (ret != 0)
        SV32_CRASH("pcthread_attr_setdetachstate failed");

    ret = pthread_create(&cthread->thread_id, &attr, cthread_fun, cthread);
    if (ret != 0)
        SV32_CRASH("pcthread_create failed");

    pthread_attr_destroy(&attr);
}

inline bool cthread_is_you(Cthread *cthread) {
    return !!pthread_equal(cthread->thread_id, pthread_self());
}
