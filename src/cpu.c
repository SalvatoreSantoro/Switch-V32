#include "cpu.h"
#include <stdint.h>
#include <stdlib.h>

VCore* vcore_create(void)
{
    return calloc(1, sizeof(VCore));
}

void vcore_load_register(VCore* core, int reg, uint32_t value)
{
    core->regs[reg] = value;
}

void vcore_destroy(VCore* core)
{
    free(core);
}
