// we're using the debug mechanism supported by the thread_mgr
// all the necessary functions are just mock to easily test all the
// step-continue-continue_all-breakpoint mechanisms without relying on
// the gdb stub and other useless complications like doing real core operations
// so we're redefining the internal function just to test the synchronization mechanisms

#include "args.h"
#include "threads_mgr.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>


Args_Context ctx = {
    .elf_stdin = NULL,
    .elf_stdout = NULL,
    .elf_stderr = NULL,
    .elf_args = NULL,
    .sdl_upscale = 0,
    .debug = true,
    .cores = 8 // debug 8 cores
};

Threads_Mgr *threads_mgr;

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
                threads_mgr_run_core(atoi(line + 1));
            else
                threads_mgr_run_all();
            break;
        case 's':
            if ((strlen(line) > 1) && (line[1] != '\n'))
                threads_mgr_step_core(atoi(line + 1));
            break;
        case 'B':
            __atomic_store_n(&breakpoint, true, __ATOMIC_RELEASE);
			break;
		case 'b':
            __atomic_store_n(&breakpoint, false, __ATOMIC_RELEASE);
			break;
        }
    }
}

void vcore_step(VCore *core) {
    struct timespec ts;
    ts.tv_sec = rand() % 5;
    ts.tv_nsec = rand() % 123456;

    int idx = thread_init();
    printf("CORE %d RUNNING\n", idx);
    thrd_sleep(&ts, NULL);
    if (__atomic_load_n(&breakpoint, __ATOMIC_ACQUIRE)) {
        printf("CORE %d STOPPING\n", idx);
        threads_mgr_halt_all();
    }
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
