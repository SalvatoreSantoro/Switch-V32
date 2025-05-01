#ifndef _RV32_SYSTEM_H
#define _RV32_SYSTEM_H

#include "cpu.h"

void ld_elf(const char *file_named, VCore *core);

void ld_elf_std(const char *stdin_name, const char *stdout_name, const char *stderr_name);

void ld_elf_args(int elf_argc, ...);

void system_call(VCore* core);


#endif
