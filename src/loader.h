#ifndef _RV32_LD_H
#define _RV32_LD_H

typedef struct Elf_File Elf_File;

Elf_File* ld_elf(const char* file_named);

#endif
