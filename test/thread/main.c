#include "cpu.h"
#include "thread.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void thread_signal_step_wrapper(Thread *thread, bool synch);
void thread_signal_start_debug_wrapper(Thread *thread, bool synch);

void vcore_reset(VCore *core) {
    memset(core, 0, sizeof(VCore));
    core->mode = SUPERVISOR_MODE;
}

void vcore_run(VCore *core) {
    // run until stopped
    while (!__atomic_load_n(&core->atomic_stop, __ATOMIC_ACQUIRE)) {
    }
}

void thread_signal_step_wrapper(Thread *thread, bool synch) {
    thread_signal_step(thread);
}

void thread_signal_start_debug_wrapper(Thread *thread, bool synch) {
    thread_signal_start_debug(thread);
}

#define NO_STATE        ((Thread_State) - 1)
#define MAX_NEXT_STATES 4

typedef void (*signal_fun)(Thread *, bool);

typedef enum {
    STOP_TRUE = 0,
    STOP_FALSE,
    HALT_TRUE,
    HALT_FALSE,
    START_TRUE,
    START_FALSE,
    SUSPEND_TRUE,
    SUSPEND_FALSE,
    STEP_TRUE,
    DEBUG_TRUE,
    IDS_NUM
} Fun_Id;

typedef struct {
    signal_fun fun;
    bool arg1;
} Test_Fun;

// clang-format off
Test_Fun functions[]={
	[STOP_TRUE] = {.fun = thread_signal_stop, .arg1 = true},
	[STOP_FALSE] = {.fun = thread_signal_stop, .arg1 = false},
	[HALT_TRUE] = {.fun = thread_signal_halt, .arg1 = true},
	[HALT_FALSE] = {.fun = thread_signal_halt, .arg1 = false},
	[START_TRUE] = {.fun = thread_signal_start, .arg1 = true},
	[START_FALSE] = {.fun = thread_signal_start, .arg1 = false},
	[SUSPEND_TRUE] = {.fun = thread_signal_suspend, .arg1 = true},
	[SUSPEND_FALSE] = {.fun = thread_signal_suspend, .arg1 = false},
	[STEP_TRUE] = {.fun = thread_signal_step_wrapper, .arg1 = true},
	[DEBUG_TRUE] = {.fun = thread_signal_start_debug_wrapper, .arg1 = true}
};

typedef struct {
    Thread_State next[MAX_NEXT_STATES];
	int comb;
} Transition;

static const Transition fsm[NUM_STATES][IDS_NUM] = {
    /* STATE_STOPPED */
    [STATE_STOPPED] = {
		[DEBUG_TRUE] = {{ STATE_HALTED, NO_STATE }, 0},
        /* all other signals lead to remaining in STATE_STOPPED */
		[HALT_TRUE]  = {{ STATE_STOPPED, NO_STATE }, 1},
		[HALT_FALSE] = {{ STATE_STOPPED, NO_STATE }, 2},
        [STOP_TRUE]  = {{ STATE_STOPPED, NO_STATE }, 3},
        [STOP_FALSE] = {{ STATE_STOPPED, NO_STATE }, 4},
        [START_TRUE] = {{ STATE_STOPPED, NO_STATE }, 5},
        [START_FALSE]= {{ STATE_STOPPED, NO_STATE }, 6},
        [SUSPEND_TRUE] = {{ STATE_STOPPED, NO_STATE }, 7},
        [SUSPEND_FALSE]= {{ STATE_STOPPED, NO_STATE }, 8},
        [STEP_TRUE] = {{ STATE_STOPPED, NO_STATE }, 9},
    },

    /* STATE_HALTED */
    [STATE_HALTED] = {
        [STOP_TRUE]   = {{ STATE_STOPPED, NO_STATE }, 10},
        [STOP_FALSE]  = {{ STATE_STOPPED, STATE_STOP_PENDING, NO_STATE }, 11},
        [START_TRUE]  = {{ STATE_STARTED, NO_STATE }, 12},
        [START_FALSE] = {{ STATE_STARTED, STATE_START_PENDING, NO_STATE }, 13},
        /* remain in HALTED for other signals */
        [HALT_TRUE]   = {{ STATE_HALTED, NO_STATE }, 14},
        [HALT_FALSE]  = {{ STATE_HALTED, NO_STATE }, 15},
        [SUSPEND_TRUE]= {{ STATE_HALTED, NO_STATE }, 16},
        [SUSPEND_FALSE]={{ STATE_HALTED, NO_STATE }, 17},
        [STEP_TRUE]   = {{ STATE_HALTED, NO_STATE }, 18},
        [DEBUG_TRUE]   = {{ STATE_HALTED, NO_STATE }, 19}
    },

    /* STATE_STARTED */
    [STATE_STARTED] = {
        [HALT_TRUE]    = {{ STATE_HALTED, NO_STATE }, 20},
        [HALT_FALSE]   = {{ STATE_STARTED, STATE_HALTED, NO_STATE }, 21},
        [SUSPEND_TRUE] = {{ STATE_SUSPENDED, NO_STATE }, 22},
        [SUSPEND_FALSE]= {{ STATE_SUSPENDED, STATE_SUSPEND_PENDING, NO_STATE }, 23},
		[STOP_TRUE]    = {{ STATE_STOPPED, NO_STATE }, 24},
		[STOP_FALSE]   = {{ STATE_STOPPED, STATE_STOP_PENDING, NO_STATE }, 25},
        /* remain in STARTED for other signals */
        [START_TRUE]   = {{ STATE_STARTED, NO_STATE }, 26},
        [START_FALSE]  = {{ STATE_STARTED, NO_STATE }, 27},
        [STEP_TRUE]    = {{ STATE_STARTED, NO_STATE }, 28},
        [DEBUG_TRUE]    = {{ STATE_STARTED, NO_STATE }, 29}
    },

    /* STATE_SUSPENDED */
    [STATE_SUSPENDED] = {
        [START_TRUE]   = {{ STATE_STARTED, NO_STATE }, 30},
        [START_FALSE]  = {{ STATE_STARTED, STATE_RESUME_PENDING, NO_STATE }, 31},
        /* remain in SUSPENDED for other signals */
        [STOP_TRUE]    = {{ STATE_SUSPENDED, NO_STATE }, 32},
        [STOP_FALSE]   = {{ STATE_SUSPENDED, NO_STATE }, 33},
        [HALT_TRUE]    = {{ STATE_SUSPENDED, NO_STATE }, 34},
        [HALT_FALSE]   = {{ STATE_SUSPENDED, NO_STATE }, 35},
        [SUSPEND_TRUE] = {{ STATE_SUSPENDED, NO_STATE }, 36},
        [SUSPEND_FALSE]= {{ STATE_SUSPENDED, NO_STATE }, 37},
        [STEP_TRUE]    = {{ STATE_SUSPENDED, NO_STATE }, 38},
        [DEBUG_TRUE]    = {{ STATE_SUSPENDED, NO_STATE }, 39}
    },


    /* STATE_STEPPING */
    [STATE_STEPPING] = {
        [HALT_FALSE] = {{ STATE_HALTED, STATE_STEPPING, NO_STATE }, 40},
        [HALT_TRUE]  = {{ STATE_HALTED, NO_STATE }, 41},
        /* remain in STEPPING for other signals */
        [STOP_TRUE]   = {{ STATE_STEPPING, NO_STATE }, 42},
        [STOP_FALSE]  = {{ STATE_STEPPING, NO_STATE }, 43},
        [START_TRUE]  = {{ STATE_STEPPING, NO_STATE }, 44},
        [START_FALSE] = {{ STATE_STEPPING, NO_STATE }, 45},
        [SUSPEND_TRUE]= {{ STATE_STEPPING, NO_STATE }, 46},
        [SUSPEND_FALSE]={{ STATE_STEPPING, NO_STATE }, 47},
        [STEP_TRUE]   = {{ STATE_STEPPING, NO_STATE }, 48},
        [DEBUG_TRUE]   = {{ STATE_STEPPING, NO_STATE }, 49}
    }
};
#define THREADS_NUM 10
#define INIT_STATES 5

Thread threads[THREADS_NUM];
int combinations[THREADS_NUM][50];
int evaluated[THREADS_NUM] = {0};

// clang-format on
bool is_transition_legal(Thread_State from, Fun_Id id, Thread_State to, int thread_id) {
    const Transition *t = &fsm[from][id];
    // used to check all STATES + SIGNALS combinations (50)

    if (combinations[thread_id][t->comb] == 0) {
        combinations[thread_id][t->comb] = 1;
        evaluated[thread_id] += 1;
    }

    for (int i = 0; i < MAX_NEXT_STATES && t->next[i] != NO_STATE; i++) {
        if (t->next[i] == to)
            return true;
    }
    return false;
}

Thread_State init_states[INIT_STATES] = {STATE_STOPPED, STATE_STARTED, STATE_HALTED, STATE_STEPPING, STATE_SUSPENDED};

int main(int argc, char *argv[]) {
    Thread_State from;
    Thread_State to;
    int id;

    srand(0);

    for (int i = 0; i < THREADS_NUM; i++) {
        // state = init_states[rand() % INIT_STATES];
        thread_init(&threads[i], (Thread_State) STATE_STOPPED);
        thread_run(&threads[i]);
    }

    for (int j = 0; j < THREADS_NUM; j++) {
        while (evaluated[j] != 50) {
            id = rand() % IDS_NUM;
            // avoid PENDING states
            do {
                from = thread_get_state(&threads[j]);
            } while ((from != STATE_STOPPED) && (from != STATE_HALTED) && (from != STATE_STEPPING) &&
                     (from != STATE_STARTED) && (from != STATE_SUSPENDED));
            functions[id].fun(&threads[j], functions[id].arg1);
            to = thread_get_state(&threads[j]);
            printf("Thread: %d, ID: %d, FROM: %d, TO: %d\n", j, id, from, to);
            assert(is_transition_legal(from, (Fun_Id) id, to, j));
        }
    }
}
