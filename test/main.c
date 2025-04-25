#include "../src/cpu.h"
#include "../src/memory.h"
#include "../src/loader.h"
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    Elf_File* file;
    VCore* core;
    uint32_t data;
    core = vcore_create();
    if ((file = ld_elf("gas", core)) == NULL)
        perror("ELF ERROR");

    data = mem_rw(core->regs[PC]);
    printf("%x\n", data);
    data = mem_rw(core->regs[PC]+4);
    printf("%x\n", data);
    data = mem_rw(core->regs[PC]+8);
    printf("%x\n", data);
    data = mem_rw(core->regs[PC]+12);
    printf("%x\n", data);





    vcore_destroy(core);
    ld_destroy_elf(file);
}
