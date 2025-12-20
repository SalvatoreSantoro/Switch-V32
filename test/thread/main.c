#include "cpu.h"
#include "cthread.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cthread_signal_step_wrapper(Cthread *thread, bool synch);
void cthread_signal_continue_wrapper(Cthread *thread, bool synch);
void cthread_signal_halt_wrapper(Cthread *thread, bool synch);

void vcore_reset(VCore *core) {
    memset(core, 0, sizeof(VCore));
    core->mode = SUPERVISOR_MODE;
}

void vcore_run(VCore *core) {
    // run until stopped
    while (!__atomic_load_n(&core->atomic_exit_loop, __ATOMIC_ACQUIRE)) {
    }
}

void cthread_signal_halt_wrapper(Cthread *thread, bool synch) {
    cthread_signal_halt(thread, synch);
}

void cthread_signal_step_wrapper(Cthread *thread, bool synch) {
    cthread_signal_step(thread);
}

void cthread_signal_continue_wrapper(Cthread *thread, bool synch) {
    cthread_signal_continue(thread);
}

#define NO_STATE        ((cthread_state) - 1)
#define MAX_NEXT_STATES 4

typedef void (*signal_fun)(Cthread *, bool);

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
    CONTINUE_TRUE,
    IDS_NUM
} Fun_Id;

typedef struct {
    signal_fun fun;
    bool arg1;
} Test_Fun;

// clang-format off
Test_Fun functions[]={
	[STOP_TRUE] = {.fun = cthread_signal_stop, .arg1 = true},
	[STOP_FALSE] = {.fun = cthread_signal_stop, .arg1 = false},
	[HALT_TRUE] = {.fun = cthread_signal_halt_wrapper, .arg1 = true},
	[HALT_FALSE] = {.fun = cthread_signal_halt_wrapper, .arg1 = false},
	[START_TRUE] = {.fun = cthread_signal_start, .arg1 = true},
	[START_FALSE] = {.fun = cthread_signal_start, .arg1 = false},
	[SUSPEND_TRUE] = {.fun = cthread_signal_suspend, .arg1 = true},
	[SUSPEND_FALSE] = {.fun = cthread_signal_suspend, .arg1 = false},
	[STEP_TRUE] = {.fun = cthread_signal_step_wrapper, .arg1 = true},
	[CONTINUE_TRUE] = {.fun = cthread_signal_continue_wrapper, .arg1 = true}
};

typedef struct {
    cthread_state next[MAX_NEXT_STATES];
	int comb;
} Transition;

static const Transition fsm[][IDS_NUM] = {
    /* STATE_STOPPED */
    [STATE_STOPPED] = {
		[CONTINUE_TRUE] = {{ STATE_STOPPED, NO_STATE }, 0},
        /* all other signals lead to remaining in STATE_STOPPED */
		[HALT_TRUE]  = {{ STATE_HALTED, NO_STATE }, 1},
		[HALT_FALSE] = {{ STATE_HALTED, STATE_HALT_PENDING, NO_STATE }, 2},
        [STOP_TRUE]  = {{ STATE_STOPPED, NO_STATE }, 3},
        [STOP_FALSE] = {{ STATE_STOPPED, NO_STATE }, 4},
        [START_TRUE] = {{ STATE_STARTED, NO_STATE }, 5},
        [START_FALSE]= {{ STATE_STARTED, STATE_START_PENDING, NO_STATE }, 6},
        [SUSPEND_TRUE] = {{ STATE_STOPPED, NO_STATE }, 7},
        [SUSPEND_FALSE]= {{ STATE_STOPPED, NO_STATE }, 8},
        [STEP_TRUE] = {{ STATE_STOPPED, NO_STATE }, 9},
    },

    /* STATE_HALTED */
    [STATE_HALTED] = {
		[CONTINUE_TRUE]   = {{STATE_CONTINUE_PENDING, NO_STATE }, 10},
		/* remain in HALTED for other signals */
        [START_TRUE]  = {{ STATE_HALTED, NO_STATE }, 11},
        [START_FALSE] = {{ STATE_HALTED, NO_STATE }, 12},
        [HALT_TRUE]   = {{ STATE_HALTED, NO_STATE }, 13},
        [HALT_FALSE]  = {{ STATE_HALTED, NO_STATE }, 14},
        [SUSPEND_TRUE]= {{ STATE_HALTED, NO_STATE }, 15},
        [SUSPEND_FALSE]={{ STATE_HALTED, NO_STATE }, 16},
        [STEP_TRUE]   = {{ STATE_HALTED, NO_STATE }, 17},
		[STOP_TRUE]   = {{ STATE_HALTED, NO_STATE }, 18},
		[STOP_FALSE]  = {{ STATE_HALTED, NO_STATE }, 19},
    },

    /* STATE_STARTED */
    [STATE_STARTED] = {
        [HALT_TRUE]    = {{ STATE_HALTED, NO_STATE }, 20},
        [HALT_FALSE]   = {{ STATE_HALT_PENDING, STATE_HALTED, NO_STATE }, 21},
        [SUSPEND_TRUE] = {{ STATE_SUSPENDED, NO_STATE }, 22},
        [SUSPEND_FALSE]= {{ STATE_SUSPENDED, STATE_SUSPEND_PENDING, NO_STATE }, 23},
		[STOP_TRUE]    = {{ STATE_STOPPED, NO_STATE }, 24},
		[STOP_FALSE]   = {{ STATE_STOPPED, STATE_STOP_PENDING, NO_STATE }, 25},
        /* remain in STARTED for other signals */
        [START_TRUE]   = {{ STATE_STARTED, NO_STATE }, 26},
        [START_FALSE]  = {{ STATE_STARTED, NO_STATE }, 27},
        [STEP_TRUE]    = {{ STATE_STARTED, NO_STATE }, 28},
        [CONTINUE_TRUE]    = {{ STATE_STARTED, NO_STATE }, 29}
    },

    /* STATE_SUSPENDED */
    [STATE_SUSPENDED] = {
        [START_TRUE]   = {{ STATE_STARTED, NO_STATE }, 30},
        [START_FALSE]  = {{ STATE_STARTED, STATE_RESUME_PENDING, NO_STATE }, 31},
		[HALT_TRUE]    = {{ STATE_HALTED, NO_STATE }, 34},
		[HALT_FALSE]   = {{ STATE_HALTED, STATE_HALT_PENDING, NO_STATE }, 35},
        /* remain in SUSPENDED for other signals */
        [STOP_TRUE]    = {{ STATE_SUSPENDED, NO_STATE }, 32},
        [STOP_FALSE]   = {{ STATE_SUSPENDED, NO_STATE }, 33},
        [SUSPEND_TRUE] = {{ STATE_SUSPENDED, NO_STATE }, 36},
        [SUSPEND_FALSE]= {{ STATE_SUSPENDED, NO_STATE }, 37},
        [STEP_TRUE]    = {{ STATE_SUSPENDED, NO_STATE }, 38},
        [CONTINUE_TRUE]    = {{ STATE_SUSPENDED, NO_STATE }, 39}
    },

    };
#define THREADS_NUM 20
#define INIT_STATES 5

Cthread cthreads[THREADS_NUM];
int combinations[THREADS_NUM][40];
int evaluated[THREADS_NUM] = {0};

// clang-format on
bool is_transition_legal(cthread_state from, Fun_Id id, cthread_state to, int cthread_id) {
    const Transition *t = &fsm[from][id];
    // used to check all STATES + SIGNALS combinations (40)

    if (combinations[cthread_id][t->comb] == 0) {
        combinations[cthread_id][t->comb] = 1;
        evaluated[cthread_id] += 1;
    }

    for (int i = 0; i < MAX_NEXT_STATES && t->next[i] != NO_STATE; i++) {
        if (t->next[i] == to)
            return true;
    }
    return false;
}

int main(int argc, char *argv[]) {
    cthread_state from;
    cthread_state to;
    int id;

    srand(100);

    for (int i = 0; i < THREADS_NUM; i++) {
        // state = init_states[rand() % INIT_STATES];
        cthread_init(&cthreads[i], (cthread_state) STATE_STOPPED);
        cthread_run(&cthreads[i]);
    }

    for (int j = 0; j < THREADS_NUM; j++) {
        while (evaluated[j] != 40) {
            id = rand() % IDS_NUM;
            //  avoid PENDING states
            do {
                from = cthread_get_state(&cthreads[j]);
            } while ((from != STATE_STOPPED) && (from != STATE_HALTED) && (from != STATE_STARTED) &&
                     (from != STATE_SUSPENDED));
            functions[id].fun(&cthreads[j], functions[id].arg1);
            to = cthread_get_state(&cthreads[j]);
			// printf("from: %d, to: %d, ID: %d\n", from, to, id);
            assert(is_transition_legal(from, (Fun_Id) id, to, j));
        }
    }
}
