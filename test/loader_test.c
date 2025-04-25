#include "../src/loader.h"
#include <stdio.h>

int main(void)
{
    Elf_File* file;
    if ((file = ld_elf("gas")) == NULL)
        perror("ELF ERROR");
}
