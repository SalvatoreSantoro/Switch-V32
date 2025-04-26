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
    VCore core = {0};
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
    while (i < 10) {
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
                break;
            case IL_TYPE:
                break;
            case S_TYPE:
                break;
            case B_TYPE:
                break;
            case J_TYPE:
                break;
            case JI_TYPE:
                break;
            case LUI:
                break;
            case AUIPC:
                break;
            case ENV_TYPE:
                break;
            }
            core.regs[PC] += 4;
        }
        i += 1;
    }
}
