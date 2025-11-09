#ifndef THREADS_MGR_H
#define THREADS_MGR_H

#include "cpu.h"
#include <pthread.h>


typedef enum {
    STOP_S,
    RUN_S,
    STEP_S,
} Thread_Signal;

typedef enum {
    STATE_HALTED,
    STATE_RUNNING,
    STATE_STEPPING
} Core_State;

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond_signal; //condition used to signal and wait when atomic_signal changes
    pthread_cond_t cond_state; //condition used to signal and wait when core_state changes
    Core_State core_state;	   //state of each core used to synchronize operations
    Thread_Signal atomic_signal; //signal sent to cores to run/stop/step them
} Thread_Cond;

typedef struct {
    pthread_t thread_id;
    Thread_Cond thread_cond;
    VCore core;
} Thread_Args;

typedef struct {
    unsigned int atomic_barrier_count; // used like a pthread_barrier_t but more flexible
    Thread_Args *threads_args;
    // NULL when debug isn't enabled
	pthread_t debug_thread;
    pthread_mutex_t halt_all_n_run_mutex; // used to make the halt_all and run/run_all mutually exclusive
                                          // In general "single" run and "single" halt, operations can race
} Threads_Mgr;

#define GET_ARG(core_idx)         threads_mgr.threads_args[core_idx]
#define GET_CORE(core_idx)        GET_ARG(core_idx).core
#define GET_THREAD_COND(core_idx) GET_ARG(core_idx).thread_cond
#define GET_THREAD_ID(core_idx)   GET_ARG(core_idx).thread_id

#define SET_SIGNAL(core_idx, val) __atomic_store_n(&GET_THREAD_COND(core_idx).atomic_signal, val, __ATOMIC_RELAXED)
#define GET_SIGNAL(core_idx)      __atomic_load_n(&GET_THREAD_COND(core_idx).atomic_signal, __ATOMIC_RELAXED)

#define SET_STATE(core_idx, val) (GET_THREAD_COND(core_idx).core_state = val)
#define GET_STATE(core_idx)      GET_THREAD_COND(core_idx).core_state

#define GET_MUTEX(core_idx)      GET_THREAD_COND(core_idx).mutex
#define GET_SIGN_COND(core_idx)  GET_THREAD_COND(core_idx).cond_signal
#define GET_STATE_COND(core_idx) GET_THREAD_COND(core_idx).cond_state

void threads_mgr_init(void);

void threads_mgr_run(void);

// low level functions that allow to specify "synchronous" or "asynchronous behaviour"
void threads_mgr_signal_run(unsigned int core_idx, bool synch);

void threads_mgr_signal_stop(unsigned int core_idx, bool synch);

void threads_mgr_signal_step(unsigned int core_idx, bool synch);

// if specified with synch = false, a return value of "false" could both means that
// the core isn't halted or that it failed the check
bool threads_mgr_is_halted(unsigned int core_idx, bool synch);

//All these function are synchronous (waiting for the action to take place before returning)
//are prefered and more robust in order to avoid race conditions

// the run functions return true if the operation succeded, they return false otherwise.
// run operations can fail in the moment they're racing with an "halt_all" operation
// since an "halt_all" operation is supposed to be triggered by a breakpoint during debugging
// we want to be sure that if an "halt_all" is racing with a "run"/"run_all" operation,
// we end up with all the cores stopped.
// So run/run_all operation should only succeed when there isn't an halt_all currently being executed
bool threads_mgr_run_core(unsigned int core_idx);

bool threads_mgr_run_all(void);

void threads_mgr_halt_core(unsigned int core_idx);

void threads_mgr_halt_all(void);

void threads_mgr_step_core(unsigned int core_idx);

#endif
