#include "defs.h"
#include "sad_gdb_internal.h"
#include "stubb_a_dub.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern Breakpoint breakpoints_g[MAX_BREAKPOINTS];
extern Sys_Ops sys_ops_g;
extern Sys_Conf sys_conf_g;
extern Builder builder_g;
extern Target_Conf target_conf_g;

brk_ret sad_insert_breakpoint(uint64_t addr) {
    uint64_t instr = 0;

    // save old instruction

    for (size_t i = 0; i < MAX_BREAKPOINTS; i++) {
        if (breakpoints_g[i].status == BRK_EMPTY) {
            // read instruction
            sys_ops_g.read_mem((byte *) (&instr), target_conf_g.brkpt_size, addr);

            // write breakpoint
            sys_ops_g.write_mem((const byte *) (&target_conf_g.breakpoint_val), sizeof(target_conf_g.brkpt_size), addr);

            breakpoints_g[i].addr = addr;
            breakpoints_g[i].instr = instr;
            breakpoints_g[i].status = BRK_FULL;
            return BRK_INSERTED;
        }
    }
    return BRK_ERROR;
}

brk_ret sad_remove_breakpoint(uint64_t addr) {
    Breakpoint *breakpoint = sad_find_breakpoint(addr);
    if (breakpoint == NULL)
        return BRK_NOT_FOUND;

    sys_ops_g.write_mem((byte *) (&breakpoint->instr), sizeof(target_conf_g.brkpt_size), addr);

    breakpoint->status = BRK_EMPTY;
    return BRK_REMOVED;
}

Breakpoint *sad_find_breakpoint(uint64_t addr) {
    for (size_t i = 0; i < MAX_BREAKPOINTS; i++) {
        //printf("SEARCHING: %lx, found: %lx\n", addr, breakpoints_g[i].addr);
		//printf("SGS %d\n", breakpoints_g[i].status);
        if ((breakpoints_g[i].addr == addr) && (breakpoints_g[i].status == BRK_FULL)) {
            return &breakpoints_g[i];
        }
    }
    return NULL;
}

// function used to step out of a breakpoint
// checks if a core is stopped on a breakpoint
// if yes just insert the instruction in the breakpoint address
// step the core, and then restore the breakpoint
brk_ret sad_step_the_breakpoint(unsigned int core_idx) {
    uint64_t pc = sys_ops_g.read_reg(core_idx, sys_conf_g.pc_id);
    //printf("THIS WAS THE PC: %lx\n", pc);

    // if removed, step and reinsert
    if (sad_remove_breakpoint(pc) == BRK_REMOVED) {
        //printf("PC NOW: %lx\n", pc);
        sys_ops_g.core_step(core_idx);

        assert(sys_ops_g.is_halted);

        sad_insert_breakpoint(pc);

        return BRK_STEPPED;
    }

    // else just return not found
    return BRK_NOT_FOUND;
}

void sad_breakpoint_reset(void) {
    for (size_t i = 0; i < MAX_BREAKPOINTS; i++)
        breakpoints_g[i].status = BRK_EMPTY;
}
