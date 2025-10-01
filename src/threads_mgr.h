#ifndef THREADS_MGR_H
#define THREADS_MGR_H


#define _XOPEN_SOURCE 600

#include "args.h"
#include "cpu.h"
#include <pthread.h>


typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
	//halted cores are always stopped on this with a pthread_cond
	//but to synchronize all the cores simultanoeusly (so can't use a mutex)
	//we always assume to first make them spin for a bit on the "atomic_stop_all"
	//then we set all the halted of the cores correctly, and after we release
	//the cores clearing atomic_stop_all, so the cores resume execution
	//but suddenly (eventually) stop on this halted variable
    bool halted;
	bool atomic_step; // Used to synchronize Debugger and Cores during stepping
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
	pthread_barrier_t barrier; // allocated only when not using debug
} Threads_Mgr;

#define GET_CORE(i) threads_mgr.threads_cores[i].core
#define GET_HALT(i) threads_mgr.halt_cond[i]
#define GET_THREAD_ID(i) threads_mgr.threads_cores[i].thread_id


void threads_mgr_init();

void threads_mgr_run();

void threads_mgr_halt_all();

void threads_mgr_continue_all();

void threads_mgr_continue_core(int core_idx);

void threads_mgr_step_core(int core_idx);

#endif
