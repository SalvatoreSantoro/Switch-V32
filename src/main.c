#include "args.h"
#include "cpu.h"
#include "debug.h"
#include "defs.h"
#include "emu.h"
#include "loader.h"
#include "memory.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "threads_mgr.h"
#include <unistd.h>

Args_Context ctx = {
    .elf_stdin = NULL, 
    .elf_stdout = NULL,
    .elf_stderr = NULL,
    .elf_args = NULL,
    .sdl_upscale = 1,
    .debug = false,
    .cores = 1
};

Threads_Mgr threads_mgr;

int main(int argc, char *argv[]) {
    // initialize global context
    ctx_init(argc, argv);

	threads_mgr_init();

//#ifdef USER

    emu_args(ctx.elf_args);

    emu_std(ctx.elf_stdin, ctx.elf_stdout, ctx.elf_stderr);

	// core 0 is always allocated in USER mode
    ld_elf(ctx.elf_name);

//#endif

    // if running an app that uses SDL, the whole virtual machine process is killed by sdl_shutdown()
	threads_mgr_run();
}
