#include "cpu.h"
#include "cthread.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void vcore_init(VCore *core, VCore_Init *init) {
    memset(core, 0, sizeof(VCore));
    core->mode = SUPERVISOR_MODE;
    if (init != NULL) {
        core->regs[PC] = init->pc;
        core->regs[SP] = init->sp;
        core->regs[A0] = init->id;
        core->regs[A1] = init->opaque;
        core->regs[GP] = init->gp;
        core->core_idx = init->id;
    }
}

void vcore_run(VCore *core) {
    // run until stopped
    while (!__atomic_load_n(&core->atomic_exit_loop, __ATOMIC_ACQUIRE)) {
    }
}


#define NO_STATE        ((cthread_state) - 1)
#define MAX_NEXT_STATES 6

typedef void (*signal_fun)(Cthread *);

// AS stands for asynchronous and SY stands for synchronous
typedef enum {
    STOP_AS,
    START_AS,
    SUSPEND_AS,
    HALT_AS,
    STEP_SY,
    CONTINUE_AS,
    IDS_NUM
} Fun_Id;

typedef struct {
    signal_fun fun;
} Test_Fun;

// clang-format off
Test_Fun functions[]={
	[STOP_AS] = {.fun = cthread_signal_stop},
	[HALT_AS] = {.fun = cthread_signal_halt},
	[START_AS] = {.fun = cthread_signal_start},
	[SUSPEND_AS] = {.fun = cthread_signal_suspend},
	[STEP_SY] = {.fun = cthread_signal_step},
	[CONTINUE_AS] = {.fun = cthread_signal_continue}
};

typedef struct {
    cthread_state next[MAX_NEXT_STATES];
	int comb;
} Transition;

static const Transition fsm[][IDS_NUM] = {
    /* STATE_STOPPED */
    [STATE_STOPPED] = {
		[CONTINUE_AS] = {{ STATE_STOPPED, NO_STATE }, 0},
		[HALT_AS] = {{ STATE_HALTED, STATE_HALT_PENDING, NO_STATE }, 1},
        [STOP_AS] = {{ STATE_STOPPED, NO_STATE }, 2},
        [START_AS]= {{ STATE_STARTED, STATE_START_PENDING, NO_STATE }, 3},
        [SUSPEND_AS]= {{ STATE_STOPPED, NO_STATE }, 4},
        [STEP_SY] = {{ STATE_STOPPED, NO_STATE }, 5},
    },

    /* STATE_HALTED */
    [STATE_HALTED] = {
		[CONTINUE_AS]   = {{STATE_CONTINUE_PENDING, STATE_SUSPENDED, STATE_STOPPED, STATE_STARTED, NO_STATE }, 6},
        [START_AS] = {{ STATE_HALTED, NO_STATE }, 7},
        [HALT_AS]  = {{ STATE_HALTED, NO_STATE }, 8},
        [SUSPEND_AS]={{ STATE_HALTED, NO_STATE }, 9},
        [STEP_SY]   = {{ STATE_HALTED, NO_STATE }, 10},
		[STOP_AS]  = {{ STATE_HALTED, NO_STATE }, 11},
    },

    /* STATE_STARTED */
    [STATE_STARTED] = {
        [HALT_AS]   = {{ STATE_HALT_PENDING, STATE_HALTED, NO_STATE }, 12},
        [SUSPEND_AS]= {{ STATE_SUSPENDED, STATE_SUSPEND_PENDING, NO_STATE }, 13},
		[STOP_AS]   = {{ STATE_STOPPED, STATE_STOP_PENDING, NO_STATE }, 14},
        [START_AS]  = {{ STATE_STARTED, NO_STATE }, 15},
        [STEP_SY]    = {{ STATE_STARTED, NO_STATE }, 16},
        [CONTINUE_AS]    = {{ STATE_STARTED, NO_STATE }, 17}
    },

    /* STATE_SUSPENDED */
    [STATE_SUSPENDED] = {
        [START_AS]  = {{ STATE_STARTED, STATE_RESUME_PENDING, NO_STATE }, 18},
		[HALT_AS]   = {{ STATE_HALTED, STATE_HALT_PENDING, NO_STATE }, 19},
        [STOP_AS]   = {{ STATE_SUSPENDED, NO_STATE }, 20},
        [SUSPEND_AS]= {{ STATE_SUSPENDED, NO_STATE }, 21},
        [STEP_SY]    = {{ STATE_SUSPENDED, NO_STATE }, 22},
        [CONTINUE_AS]    = {{ STATE_SUSPENDED, NO_STATE }, 23}
    },

    };

#define NUM_COMBINATIONS 24
#define THREADS_NUM 50
#define INIT_STATES 5

Cthread cthreads[THREADS_NUM];
int combinations[THREADS_NUM][NUM_COMBINATIONS];
int evaluated[THREADS_NUM] = {0};

// clang-format on
bool is_transition_legal(cthread_state from, Fun_Id id, cthread_state to, int cthread_id) {
    const Transition *t = &fsm[from][id];
    // used to check all STATES + SIGNALS combinations (NUM_COMBINATIONS)

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
        cthread_init(&cthreads[i], (cthread_state) STATE_STOPPED, NULL);
        cthread_run(&cthreads[i]);
    }

    for (int j = 0; j < THREADS_NUM; j++) {
        while (evaluated[j] != NUM_COMBINATIONS) {
            id = rand() % IDS_NUM;
            //  avoid PENDING states
            do {
                from = cthread_get_state(&cthreads[j]);
            } while ((from != STATE_STOPPED) && (from != STATE_HALTED) && (from != STATE_STARTED) &&
                     (from != STATE_SUSPENDED));
            functions[id].fun(&cthreads[j]);
            to = cthread_get_state(&cthreads[j]);
            //printf("from: %d, to: %d, ID: %d\n", from, to, id);
            assert(is_transition_legal(from, (Fun_Id) id, to, j));
        }
    }
}
