#include "../src/cpu.h"
#include "../src/loader.h"
#include "../src/memory.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    Elf_File* file;
    VCore core = { 0 };
    uint32_t ins;
    if ((file = ld_elf("gas", &core)) == NULL) {
        perror("ELF ERROR");
        return EXIT_FAILURE;
    }
    // kept file allocated on heap just in case
    // if i find out that the loader is useless after loading the file
    // could just deallocate it directly in ld_elf
    ld_destroy_elf(file);

    int i = 0;
    while (i < 1000) {
        getchar();
        ins = mem_rw(core.regs[PC]);
        printf("%x\n", ins);
        if (IS_COMPRESSED(ins)) {
            printf("Compressed\n");
            core.regs[PC] += 2;
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
                continue;
                break;
            case LUI:
                vcore_lui_type(&core, ins);
                break;
            case AUIPC:
                vcore_auipc_type(&core, ins);
                break;
            case ENV_TYPE:
                break;
            default:
                fprintf(stderr, "%x BADOPCODE\n", ins);
                break;
            }
            core.regs[PC] += 4;
        }
        i += 1;
    }


    /* printf("%x\n", core.regs[REG_ZERO]); */
    /* printf("%x\n", core.regs[REG_RA]); */
    /* printf("%x\n", core.regs[REG_SP]); */
    /* printf("%x\n", core.regs[REG_GP]); */
    /* printf("%x\n", core.regs[REG_TP]); */
    /* printf("T0: %x\n", core.regs[REG_T0]); */
    /* printf("%x\n", core.regs[REG_T1]); */
    /* printf("%x\n", core.regs[REG_T2]); */
    /* printf("%x\n", core.regs[REG_T3]); */
    /* printf("%x\n", core.regs[REG_S0]); */
    /* printf("%x\n", core.regs[REG_S1]); */
    /* printf("%x\n", core.regs[REG_A0]); */
    /* printf("%x\n", core.regs[REG_A1]); */
    /* printf("%x\n", core.regs[REG_A2]); */
    /* printf("%x\n", core.regs[REG_A3]); */
    /* printf("%x\n", core.regs[REG_A4]); */
    /* printf("%x\n", core.regs[REG_A5]); */
    /* printf("%x\n", core.regs[REG_A6]); */
    /* printf("%x\n", core.regs[REG_A7]); */
    /**/
    /* printf("%x\n", core.regs[REG_S2]); */
    /* printf("%x\n", core.regs[REG_S3]); */
    /* printf("%x\n", core.regs[REG_S4]); */
    /* printf("%x\n", core.regs[REG_S5]); */
    /* printf("%x\n", core.regs[REG_S6]); */
    /* printf("%x\n", core.regs[REG_S7]); */
    /* printf("%x\n", core.regs[REG_S8]); */
    /* printf("%x\n", core.regs[REG_S9]); */
    /* printf("%x\n", core.regs[REG_S10]); */
    /* printf("%x\n", core.regs[REG_S11]); */
    /**/
    /**/
    /* printf("%x\n", core.regs[REG_T3]); */
    /* printf("%x\n", core.regs[REG_T4]); */
    /* printf("%x\n", core.regs[REG_T5]); */
    /* printf("%x\n", core.regs[REG_T6]); */
    /**/
    /**/
    /* printf("%x\n", core.regs[PC]); */

    printf("%d\n", *(uint32_t*)core.regs[REG_T0]);

}
