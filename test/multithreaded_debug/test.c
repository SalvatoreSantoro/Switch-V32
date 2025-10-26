// we're using the debug mechanism supported by the thread_mgr
// all the necessary functions are just mock to easily test all the
// step-continue-continue_all-breakpoint mechanisms without relying on
// the gdb stub and other useless complications like doing real core operations
// so we're redefining the internal function just to test the synchronization mechanisms

#include "args.h"
#include "threads_mgr.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <valgrind/helgrind.h>

Args_Context ctx = {
    .elf_stdin = NULL,
    .elf_stdout = NULL,
    .elf_stderr = NULL,
    .elf_args = NULL,
    .sdl_upscale = 0,
    .debug = true,
    .cores = 4 // debug 8 cores
};

Threads_Mgr threads_mgr;

bool breakpoint = false;

// MOCKS
void debug_thread_fun() {
    char line[10];
    while (1) {
        if (fgets(line, sizeof(line), stdin) == NULL)
            exit(1);
        switch (line[0]) {
        case 'c':
            if ((strlen(line) > 1) && (line[1] != '\n'))
                threads_mgr_run_core((unsigned int) atoi(line + 1));
            else {
                threads_mgr_run_all();
            }
            break;
        case 's':
            if ((strlen(line) > 1) && (line[1] != '\n'))
                threads_mgr_step_core(atoi(line + 1));
            break;

        case 'B':
			ANNOTATE_IGNORE_READS_AND_WRITES_BEGIN();
            __atomic_store_n(&breakpoint, true, __ATOMIC_RELAXED);
			ANNOTATE_IGNORE_READS_AND_WRITES_END();
            break;
        case 'b':
			ANNOTATE_IGNORE_READS_AND_WRITES_BEGIN();
            __atomic_store_n(&breakpoint, false, __ATOMIC_RELAXED);
			ANNOTATE_IGNORE_READS_AND_WRITES_END();
            break;
        }
    }
}

void vcore_step(VCore *core) {
    struct timespec ts;
    ts.tv_sec = rand() % 5;
    ts.tv_nsec = rand() % 123456;

    unsigned int idx = core->core_idx;
    printf("CORE %u RUNNING\n", idx);
    thrd_sleep(&ts, NULL);
    ANNOTATE_IGNORE_READS_AND_WRITES_BEGIN();
    if (__atomic_load_n(&breakpoint, __ATOMIC_RELAXED)) {
        printf("CORE %u STOPPING\n", idx);
        threads_mgr_halt_all();
    }
	ANNOTATE_IGNORE_READS_AND_WRITES_END();
    return;
}

// useless
void vcore_run(VCore *core) {
    return;
}

int main(int argc, char *argv[]) {
    threads_mgr_init();
    threads_mgr_run();
}
