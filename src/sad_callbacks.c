#include "sad_callbacks.h"
#include "cpu.h"
#include "memory.h"
#include <string.h>

// core_id unused for now
void read_regs(byte *output, size_t output_sz, int core_id) {
    size_t regs_size = output_sz - 4; // don't count PC
    // copy all regs
    memcpy(output, core.regs, regs_size);
    // copy PC
    memcpy(output + regs_size, &core.pc, 4);
}

// core_id unused for now
void write_regs(const byte *input, size_t input_sz, int core_id) {
    size_t regs_size = input_sz - 4; // don't count PC
    // copy all regs
    memcpy(core.regs, input, regs_size);
    // copy PC
    memcpy(&core.pc, input + regs_size, 4);
}

void read_mem(byte *output, size_t output_sz, uint32_t addr) {
    mem_rb_ptr_s(addr, output, output_sz);
}

void write_mem(const byte *input, size_t input_sz, uint32_t addr) {
    mem_wb_ptr_s(addr, input, input_sz);
}

void core_step(int core_id){
    vcore_step(&core);
}
