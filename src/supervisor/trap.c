#include "trap.h"
#include "cpu.h"
#include "csr.h"
#include <assert.h>
#include <stdint.h>

#define GET_INTERRUPT_CODE(code) ((uint32_t) code & 0x7FFFFFFFu)

void dispatch_trap(VCore *core, trap_code code, uint32_t faulting_val) {
    // save old Interrupt Enable value and then reset interrupts if enabled
    if (SSTATUS_SIE(core->regs[SSTATUS])) { // SIE 1
        SET_SSTATUS_SPIE(core->regs[SSTATUS]);
        // disable interrupts
        RESET_SSTATUS_SIE(core->regs[SSTATUS]);
    } else { // SIE 0
        SET_SSTATUS_SPIE(core->regs[SSTATUS]);
    }

    // interrupt code automatically set because "code" is already set in case
    // of an interrupt
    core->regs[SCAUSE] = (uint32_t) code;

    // save old privilege mode
    if (core->mode == SUPERVISOR_MODE)
        SET_SSTATUS_SPP(core->regs[SSTATUS]);
    else if (core->mode == USER_MODE) {
        RESET_SSTATUS_SPP(core->regs[SSTATUS]);
        core->mode = SUPERVISOR_MODE;
    }

    // save interrupted pc/pc that cause exception
    core->regs[SEPC] = core->regs[PC];

    if ((int) code >= 0) { // exceptions
        // set exception value if used (used only for exceptions)
        core->regs[STVAL]= faulting_val;
        // compute new PC
        core->regs[PC]= STVEC_BASE(core->regs[STVEC]);
    } else { // interrupts
        if (STVEC_MODE(core->regs[STVEC]) == STVEC_MODE_DIRECT)
            core->regs[PC] = STVEC_BASE(core->regs[STVEC]);
        else if (STVEC_MODE(core->regs[STVEC]) == STVEC_MODE_VECTORED)
            core->regs[PC] = STVEC_BASE(core->regs[STVEC]) + (GET_INTERRUPT_CODE(code) * 4);
        else
            assert(0 && "THERE IS A BUG IN STVEC MODE");
    }
}

// LCOFI unimplemented
void check_interrupts(VCore *core) {
    // never process interrupts during LL/SC instructions
    if (core->ll_sc_flag)
        return;

    // if in supervisor mode ignore interrupts if SIE bit in SSTATUS is 0
    if ((core->mode == SUPERVISOR_MODE) && !SSTATUS_SIE(core->regs[SSTATUS]))
        return;

    // order specified in the manual to check SEI, SSI, STI, LCOFI
    if (SIE_SEIE(core->regs[SIE]) && SIP_SEIP(core->regs[SIP]))
        dispatch_trap(core, SUPERVISOR_EXTERNAL_INTERRUPT, RESET_FAULT_VAL);

    if (SIE_SSIE(core->regs[SIE]) && SIP_SSIP(core->regs[SIP]))
        dispatch_trap(core, SUPERVISOR_SOFTWARE_INTERRUPT, RESET_FAULT_VAL);

    if (SIE_STIE(core->regs[SIE]) && SIP_STIP(core->regs[SIP]))
        dispatch_trap(core, SUPERVISOR_TIMER_INTERRUPT, RESET_FAULT_VAL);
}
