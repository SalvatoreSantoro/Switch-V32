#include "args.h"
#include "cpu.h"
#include "emu.h"
#include "loader.h"
#include "memory.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    VCore core = {0};
    uint32_t ins;

    // initialize global context
    ctx_init(argc, argv);

    // IMPORTANT: RIGHT NOW WE'RE ASSUMING ARGC, ARGV FITS INTO 1 PAGE (4096 BYTES)
    emu_args(ctx.elf_argc, ctx.elf_argvs);
    emu_std(ctx.elf_stdin, ctx.elf_stdout, ctx.elf_stderr);

    ld_elf(ctx.elf_name, &core);

    //if running an app that uses SDL, the whole virtual machine process is killed by sdl_shutdown()
    while (1) {
        core.regs[ZERO] = 0;
        ins = mem_rw(core.pc);
        if (IS_COMPRESSED(ins)) {
            printf("Compressed unimplemented\n");
            exit(EXIT_FAILURE);
            core.pc += 2;
        } else {
            switch (OPCODE_TYPE(ins)) {
            case R_TYPE:
                vcore_r_type(&core, ins);
                break;
            case IR_TYPE:
                vcore_ir_type(&core, ins);
                break;
            case IL_TYPE:
                vcore_il_type(&core, ins);
                break;
            case S_TYPE:
                vcore_s_type(&core, ins);
                break;
            case B_TYPE:
                vcore_b_type(&core, ins);
                continue; // skip the PC + 4
                break;
            case J_TYPE:
                vcore_j_type(&core, ins);
                continue; // skip the PC + 4
                break;
            case IJ_TYPE:
                vcore_ij_type(&core, ins);
                continue; // skip the PC + 4
                break;
            case LUI:
                vcore_lui_type(&core, ins);
                break;
            case AUIPC:
                vcore_auipc_type(&core, ins);
                break;
            case A_TYPE:
                vcore_a_type(&core, ins);
                break;
            case ENV_TYPE:
                emu_system_call(&core);
                break;
            default:
                fprintf(stderr, "%x BADOPCODE at %x\n", ins, core.pc);
                exit(EXIT_FAILURE);
                break;
            }
            core.pc += 4;
        }
    }
}
