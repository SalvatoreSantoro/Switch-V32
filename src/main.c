#include "args.h"
#include "cpu.h"
#include "defs.h"
#include "emu.h"
#include "loader.h"
#include "memory.h"
#include "sad_callbacks.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    uint32_t ins;

    // initialize global context
    ctx_init(argc, argv);

    // the layout is |STACK_BASE|ARGC|ARGV[0]|ARGV[1]|...|ARGV[ARGC-1]|NULL|STR0|STR1|...|STR_ARGC-1|
    emu_args(ctx.elf_args);

    emu_std(ctx.elf_stdin, ctx.elf_stdout, ctx.elf_stderr);

    ld_elf(ctx.elf_name, &core);

    if (ctx.debug) {
        Stub_Conf conf = {
            .sys_conf.arch = RV32,
            .sys_conf.regs_num = REG_NUMS + 1, // PC isn't counted in regs
            .sys_conf.smp = 15,                 // core numbers
            .sys_ops.read_regs = read_regs,
            .sys_ops.write_regs = write_regs,
            .sys_ops.read_mem = read_mem,
            .sys_ops.write_mem = write_mem,
            .sys_ops.core_step = core_step,
            .port = STUB_PORT,
            .buffers_size = STUB_BUFF_SIZE,
            .socket_io_size = STUB_READ_SIZE,
        };

        sad_stub_init(conf);
        printf("Running GDB STUB on port: %d\n", STUB_PORT);
        sad_stub_handle_cmds();
    }

    // if running an app that uses SDL, the whole virtual machine process is killed by sdl_shutdown()
    vcore_run(&core);
}
