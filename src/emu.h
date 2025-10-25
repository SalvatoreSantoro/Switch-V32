#ifndef _SV32_EMU_H
#define _SV32_EMU_H

#include "cpu.h"

void emu_std(void);

void emu_args(void);

void emu_system_call(VCore *core);

#endif
