#include "../src/cpu.h"
#include "../src/loader.h"
#include "../src/memory.h"
#include "../src/system.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    VCore core = {0};
    uint32_t ins;

    ld_elf("gas", &core);

    printf("BRK: %x\n", core.brk);
    printf("STACK: %x\n", core.regs[SP]);

    while (1) {
        // RESET ZERO REGISTER
        core.regs[ZERO] = 0;
        ins = mem_rw(core.pc);
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
                fprintf(stderr, "%x BADOPCODE\n", ins);
                exit(EXIT_FAILURE);
                break;
            }
            core.pc += 4;
        }
    }
}
