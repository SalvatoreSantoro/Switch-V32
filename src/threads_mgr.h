#ifndef THREADS_MGR_H
#define THREADS_MGR_H

#include "cpu.h"
#include <pthread.h>

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
    pthread_cond_t cond_signal;
    pthread_cond_t cond_state;
    // halted cores are always stopped on this with a pthread_cond
    // but to synchronize all the cores simultanoeusly (so can't use a mutex)
    // we always assume to first make them spin for a bit on the "atomic_stop_all"
    // then we set all the halted of the cores correctly, and after we release
    // the cores clearing atomic_stop_all, so the cores resume execution
    // but suddenly (eventually) stop on this halted variable

    // we're making this atomic even if it's wrapped in mutexes
    // because it's read atomically in vcore_run when running as SUPERVISOR (when not debugging)
    // (avoiding to lock/unlock in the hot loop)
    Core_State core_state;
    Thread_Signal atomic_signal;
} Thread_Cond;

typedef struct {
    pthread_t thread_id;
    Thread_Cond thread_cond;
    VCore core;
} Thread_Args;

typedef struct {
    unsigned int atomic_barrier_count; // used like a pthread_barrier_t but more flexible
    pthread_t debug_thread;
    Thread_Args *threads_args;
    // NULL when debug isn't enabled
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

void threads_mgr_halt_all(void);

void threads_mgr_halt_core(unsigned int core_idx);

void threads_mgr_run_all(void);

void threads_mgr_run_core(unsigned int corb_idx);

void threads_mgr_step_core(unsigned int core_idx);

bool threads_mgr_is_halted(unsigned int core_idx, bool blocking);

// TESTING INTERFACE //
// exposing them just to make testing easier not meant to be used

void barrier_count_wait(void);

unsigned int thread_init(void);

// void *debug_core_thread_fun(void *args);

#endif
