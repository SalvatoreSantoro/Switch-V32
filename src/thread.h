#ifndef THREAD_H
#define THREAD_H

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
	NUM_STATES
} Thread_State;

typedef enum {
    START_S = STATE_STARTED,
    STOP_S = STATE_STOPPED,
    SUSPEND_S = STATE_SUSPENDED,
    HALT_S = STATE_HALTED,
    STEP_S = STATE_STEPPING,
    START_DEBUG_S,
} Thread_Signal;

typedef struct {
    pthread_t thread_id;
    pthread_mutex_t mutex;
    pthread_cond_t cond_signal; // condition used to signal and wait when atomic_signal changes
    pthread_cond_t cond_state;  // condition used to signal and wait when core_state changes
    Thread_State state;         // state of each core used to synchronize operations
    Thread_Signal signal;       // signal sent to cores to run/stop/step them
    VCore core;
} Thread;

void thread_signal_stop(Thread *thread, bool synch);

void thread_signal_halt(Thread *thread, bool synch);

void thread_signal_start(Thread *thread, bool synch);

void thread_signal_suspend(Thread *thread, bool synch);

Thread_State thread_get_state(Thread *thread);

// always synchronized
void thread_signal_step(Thread *thread);

void thread_signal_start_debug(Thread *thread);

void thread_init(Thread *thread, Thread_State state);

void thread_run(Thread *thread);

#endif
