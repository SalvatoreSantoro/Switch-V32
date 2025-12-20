#include "args.h"
#include "cthread.h"
#ifdef USER
    #include "emu.h"
#endif
#include "debug.h"
#include "loader.h"
#include "memory.h"
#include "threads_mgr2.h"
#include <stdint.h>
#include <unistd.h>

Args_Context ctx = {.elf_stdin = NULL,
                    .elf_stdout = NULL,
                    .elf_stderr = NULL,
                    .elf_args = NULL,
                    .sdl_upscale = 1,
                    .cores = 1,
                    .memory_size = DEF_MEM_SIZE,
                    .stack_base = DEF_STACK_BASE,
#ifdef USER
                    .brk_limit = DEF_BRK_LIMIT,
#endif
                    .debug = false,
                    .binary = false};

Threads_Mgr threads_mgr;

int main(int argc, char *argv[]) {
    // initialize global context
    ctx_init(argc, argv);

    // initialize memory
    mem_init();

    threads_mgr_init();

#ifdef USER
    emu_args();

    emu_std();
#endif

    // ctx.binary is always ignored if running in USER
    if (ctx.binary)
        // assuming that the binary initializes the STACK
        ld_bin(&threads_mgr.cthreads[0].core);
    else
        ld_elf(&threads_mgr.cthreads[0].core);


    if (ctx.debug) {
		run_debug();
    } else {
		// if running an app that uses SDL, the whole virtual machine process is killed by sdl_shutdown()
        cthread_signal_continue(&threads_mgr.cthreads[0]);
    }

}
