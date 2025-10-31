#include "cpu.h"
#include "trap.h"

void vcore_sys_type(VCore *core, uint32_t ins) {
    uint32_t imm = I_IMM(ins);
    switch (imm) {
    case ECALL:
        dispatch_trap(core, ECALL_U_MODE, RESET_FAULT_VAL);
        core->pc += 4;
        break;
    case EBREAK:
        // never increase PC, we want that the core stops on the breakpoint
        // if debugging. If the breakpoint is an exception INSIDE an USER program
        // running on a SUPERVISOR, we also don't want to increment because the dispatch exception
        // will change the PC so that the next instruction is the first of the exception handler
        dispatch_trap(core, BRKPT, core->pc);
        break;
    default:
        dispatch_trap(core, ILL_INS, ins);
        break;
    }
}
