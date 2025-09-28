#ifndef DEBUG_H
#define DEBUG_H

#include "stub.h"

void read_regs(byte *output, size_t output_sz, int core_id);

void write_regs(const byte *input, size_t input_sz, int core_id);

void read_mem(byte *output, size_t output_sz, uint32_t addr);

void write_mem(const byte *input, size_t input_sz, uint32_t addr);

void core_step(int core_id);

void *debug_thread_fun(void *args);

#endif // !DEBUG_H
