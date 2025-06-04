#ifndef _RV32_EMU_H
#define _RV32_EMU_H

#include "cpu.h"
#include <stdio.h>

void emu_std(const char *stdin_name, const char *stdout_name, const char *stderr_name);

void emu_args(int elf_argc, va_list ap);

void emu_system_call(VCore *core);

#endif
