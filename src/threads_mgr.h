#ifndef THREADS_MGR_H
#define THREADS_MGR_H

#include "args.h"
#include "cpu.h"
#include <pthread.h>

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool halted;
} Halt_Cond;

typedef struct{
	VCore core;
	pthread_t thread_id;
} Thread;

typedef struct {
	bool atomic_stop_all; // used by debugging thread to stop all the cores at the same time
    Halt_Cond *halt_cond; // NULL when debug isn't enabled
	Thread* threads_cores;
	pthread_t debug_thread;
} Threads_Mgr;

void threads_mgr_init();

void threads_mgr_run();

#endif
