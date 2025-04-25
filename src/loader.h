#ifndef _RV32_LD_H
#define _RV32_LD_H

#include "cpu.h"

typedef struct Elf_File Elf_File;

Elf_File* ld_elf(const char* file_named, VCore* core);

void ld_destroy_elf(Elf_File* elf);

#endif
