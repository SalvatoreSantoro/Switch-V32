#include "../src/cpu.h"
#include "../src/memory.h"
#include "../src/system.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    // take them as args
    const char *elf_stdin = NULL;
    const char *elf_stdout = "out.txt";
    const char *elf_stderr = "err.txt";
    int elf_argc = 1;
    const char *elf_argv1 = "./doom";
    const char *elf_argv2 = NULL;
    const char *elf_argv3 = NULL;

    VCore core = {0};
    uint32_t ins;

    // IMPORTANT: RIGHT NOW WE'RE ASSUMING EVERYTHING FITS INTO 1 PAGE (4096 BYTES)
    ld_elf_args(elf_argc, elf_argv1, elf_argv2, elf_argv3);

    ld_elf("doom-riscv.elf", &core);

    ld_elf_std(elf_stdin, elf_stdout, elf_stderr);

    // int activated = 0;
    while (1) {
        // if (activated)
        // getchar();
        //   RESET ZERO REGISTER
        core.regs[ZERO] = 0;
        ins = mem_rw(core.pc);
        /* printf("BRK: %x\n", core.elf_brk); */
        /* printf("STACK: %x\n", core.regs[SP]); */
        /* printf("PC: %x\n", core.pc); */
        /* printf("INS: %x\n", ins); */
        // if (ins == 0xe1010113)
        //   activated = 1;
        if (IS_COMPRESSED(ins)) {
            printf("Compressed\n");
            core.pc += 2;
        } else {
            // printf("Uncompressed\n");
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
            case ENV_TYPE:
                system_call(&core);
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
