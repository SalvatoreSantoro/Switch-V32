#ifndef Ccthread_H
#define Ccthread_H

#include "cpu.h"
#include <pthread.h>

// keeping it compatible with SBI HSM values
typedef enum {
    STATE_STARTED = 0,
    STATE_STOPPED = 1,
    STATE_START_PENDING = 2,
    STATE_STOP_PENDING = 3,
    STATE_SUSPENDED = 4,
    STATE_SUSPEND_PENDING = 5,
    STATE_RESUME_PENDING = 6,
    STATE_HALTED = 7,
    STATE_STEPPING = 8,
    STATE_HALT_PENDING = 10,
	STATE_CONTINUE_PENDING = 11
} cthread_state;

typedef enum {
    START_S = STATE_STARTED,
    STOP_S = STATE_STOPPED,
    SUSPEND_S = STATE_SUSPENDED,
    HALT_S = STATE_HALTED,
    STEP_S = STATE_STEPPING,
    CONTINUE_S,
} cthread_signal;

typedef struct {
    pthread_t thread_id;
    pthread_mutex_t mutex;
    pthread_cond_t cond_signal; // condition used to signal and wait when atomic_signal changes
    pthread_cond_t cond_state;  // condition used to signal and wait when core_state changes
    cthread_state state;         // state of each core used to synchronize operations
    cthread_signal signal;       // signal sent to cores to run/stop/step them
    cthread_signal old_signal;   // used to restore old state after halting during debug
    VCore core;
} Cthread;


void cthread_init(Cthread *thread, cthread_state state);

void cthread_run(Cthread *thread);

cthread_state cthread_get_state(Cthread *thread);

// HSM
// always asynch
void cthread_signal_stop(Cthread *thread);

void cthread_signal_start(Cthread *thread);

void cthread_signal_suspend(Cthread *thread);

// DEBUG
void cthread_signal_halt(Cthread *thread, bool synch);

// always synchronized
void cthread_signal_step(Cthread *thread);

void cthread_signal_continue(Cthread *thread);

cthread_state cthread_get_hsm_state(Cthread *thread);

bool cthread_is_you(Cthread *thread);

#endif
