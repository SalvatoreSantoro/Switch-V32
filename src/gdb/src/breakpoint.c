#include "sad_gdb_internal.h"
#include <stdint.h>
#include <stdio.h>

extern SAD_Stub server;

bool sad_insert_breakpoint(uint32_t addr) {
    uint32_t instr;
    bool inserted = false;
    static const uint32_t riscv_breakpoint = 0x00100073;

    // save old instruction

    for (size_t i = 0; i < MAX_BREAKPOINTS; i++) {
        if (server.breakpoints[i].status == BRK_EMPTY) {
            // read instruction
            server.sys_ops.read_mem((byte *) (&instr), sizeof(instr), addr);

            // write breakpoint
            server.sys_ops.write_mem((byte *) (&riscv_breakpoint), sizeof(riscv_breakpoint), addr);

            server.breakpoints[i].addr = addr;
            server.breakpoints[i].instr = instr;
            server.breakpoints[i].status = BRK_FULL;
            inserted = true;
            break;
        }
    }

    return inserted;
}

bool sad_remove_breakpoint(uint32_t addr) {
    Breakpoint *breakpoint = sad_find_breakpoint(addr);
    if (breakpoint == NULL)
        return false;

    server.sys_ops.write_mem((byte *) (&breakpoint->instr), sizeof(breakpoint->instr), addr);
    breakpoint->status = BRK_EMPTY;
	return true;
}

Breakpoint *sad_find_breakpoint(uint32_t addr) {
    for (size_t i = 0; i < MAX_BREAKPOINTS; i++) {
        if ((server.breakpoints[i].addr == addr) && (server.breakpoints[i].status == BRK_FULL)) {
            return &server.breakpoints[i];
        }
    }
    return NULL;
}

// function used to step out of a breakpoint
// checks if a core is stopped on a breakpoint
// if yes just insert the instruction in the breakpoint address
// step the core, and then restore the breakpoint
bool sad_step_the_breakpoint(int core_idx) {
	size_t regs_sz = server.builder.cached_regs_bytes;
    byte regs[regs_sz];
    server.sys_ops.read_regs(regs, regs_sz, core_idx);

    // get the PC (the last of the registers)
    uint32_t pc = *((uint32_t *) (regs + server.builder.cached_regs_bytes - 4));
	
	// if removed, step and reinsert
	if (sad_remove_breakpoint(pc)){
		server.sys_ops.core_step(core_idx);
		sad_insert_breakpoint(pc);
	}
	// else just return false
	return false;
}
