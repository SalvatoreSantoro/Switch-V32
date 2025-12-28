#ifndef THREADS_MGR_H
#define THREADS_MGR_H

#include "cthread.h"
#include <pthread.h>

typedef struct {
    Cthread *cthreads;
    pthread_mutex_t halt_all_n_run_mutex; // used to make the halt_all and run/run_all mutually exclusive
                                          // In general "single" run and "single" halt, operations can race
} Threads_Mgr;


extern Threads_Mgr threads_mgr;

void threads_mgr_init(VCore_Init *core0_init);

void threads_mgr_run(void);

void threads_mgr_continue_core(unsigned int core_idx);

void threads_mgr_continue_cores(void);

void threads_mgr_halt_cores(void);

void threads_mgr_step_core(unsigned int core_idx);

bool threads_mgr_are_halted(void);

#endif
