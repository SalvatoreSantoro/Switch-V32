#include "sad_gdb_internal.h"
#include "stubb_a_dub.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern SAD_Stub server;

brk_ret sad_insert_breakpoint(uint32_t addr) {
    uint32_t instr;
    static const uint32_t riscv_breakpoint = 0x00100073;

    // save old instruction

    for (size_t i = 0; i < MAX_BREAKPOINTS; i++) {
        if (server.breakpoints[i].status == BRK_EMPTY) {
            // read instruction
            if (server.sys_ops.read_mem((byte *) (&instr), sizeof(instr), addr) != SYS_OK)
                break;

            // write breakpoint
            if (server.sys_ops.write_mem((const byte *) (&riscv_breakpoint), sizeof(riscv_breakpoint), addr) != SYS_OK)
                break;

            server.breakpoints[i].addr = addr;
            server.breakpoints[i].instr = instr;
            server.breakpoints[i].status = BRK_FULL;
            return BRK_INSERTED;
        }
    }
    return BRK_ERROR;
}

brk_ret sad_remove_breakpoint(uint32_t addr) {
    Breakpoint *breakpoint = sad_find_breakpoint(addr);
    if (breakpoint == NULL)
        return BRK_NOT_FOUND;

    if (server.sys_ops.write_mem((byte *) (&breakpoint->instr), sizeof(breakpoint->instr), addr) != SYS_OK)
        return BRK_ERROR;

    breakpoint->status = BRK_EMPTY;
    return BRK_REMOVED;
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
brk_ret sad_step_the_breakpoint(unsigned int core_idx) {
    size_t regs_sz = server.builder.cached_regs_bytes;
    sys_err step_result;
    byte regs[regs_sz];
    if (server.sys_ops.read_regs(regs, regs_sz, core_idx) != SYS_OK)
        return BRK_ERROR;

    // get the PC (the last of the registers)
    uint32_t pc;
    memcpy(&pc, regs + server.builder.cached_regs_bytes - 4, sizeof(pc));

    // if removed, step and reinsert
    if (sad_remove_breakpoint(pc) == BRK_REMOVED) {
        step_result = server.sys_ops.core_step(core_idx);
        sad_insert_breakpoint(pc);
        if (step_result == SYS_OK)
            return BRK_STEPPED;
		else
			return BRK_ERROR;
    }

    // else just return not found
    return BRK_NOT_FOUND;
}
