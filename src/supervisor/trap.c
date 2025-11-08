#include "trap.h"
#include "cpu.h"
#include "csr.h"
#include <assert.h>
#include <stdint.h>

#define GET_INTERRUPT_CODE(code) ((uint32_t) code & 0x7FFFFFFFu)

void dispatch_trap(VCore *core, trap_code code, uint32_t faulting_val) {
    // save old Interrupt Enable value and then reset interrupts if enabled
    if (SSTATUS_SIE(core->sstatus)) { // SIE 1
        SET_SSTATUS_SPIE(core->sstatus);
        // disable interrupts
        RESET_SSTATUS_SIE(core->sstatus);
    } else { // SIE 0
        SET_SSTATUS_SPIE(core->sstatus);
    }

    // interrupt code automatically set because "code" is already set in case
    // of an interrupt
    core->scause = (uint32_t) code;

    // save old privilege mode
    if (core->mode == SUPERVISOR_MODE)
        SET_SSTATUS_SPP(core->sstatus);
    else if (core->mode == USER_MODE) {
        RESET_SSTATUS_SPP(core->sstatus);
        core->mode = SUPERVISOR_MODE;
    }

    // save interrupted pc/pc that cause exception
    core->sepc = core->pc;

    if ((int) code >= 0) { // exceptions
        // set exception value if used (used only for exceptions)
        core->stval = faulting_val;
        // compute new PC
        core->pc = STVEC_BASE(core->stvec);
    } else { // interrupts
        if (STVEC_MODE(core->stvec) == STVEC_MODE_DIRECT)
            core->pc = STVEC_BASE(core->stvec);
        else if (STVEC_MODE(core->stvec) == STVEC_MODE_VECTORED)
            core->pc = STVEC_BASE(core->stvec) + (GET_INTERRUPT_CODE(code) * 4);
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
    if ((core->mode == SUPERVISOR_MODE) && !SSTATUS_SIE(core->sstatus))
        return;

    // order specified in the manual to check SEI, SSI, STI, LCOFI
    if (SIE_SEIE(core->sie) && SIP_SEIP(core->sip))
        dispatch_trap(core, SUPERVISOR_EXTERNAL_INTERRUPT, RESET_FAULT_VAL);

    if (SIE_SSIE(core->sie) && SIP_SSIP(core->sip))
        dispatch_trap(core, SUPERVISOR_SOFTWARE_INTERRUPT, RESET_FAULT_VAL);

    if (SIE_STIE(core->sie) && SIP_STIP(core->sip))
        dispatch_trap(core, SUPERVISOR_TIMER_INTERRUPT, RESET_FAULT_VAL);
}
