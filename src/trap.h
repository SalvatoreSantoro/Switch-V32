#ifndef SV32_TRAP_H
#define SV32_TRAP_H

#include "cpu.h"
#include <stdint.h>
typedef enum {
    INS_ADDR_MIS_ALGN = 0,
    INS_ACC_FAULT = 1,
    ILL_INS = 2,
    BRKPT = 3,
    LOAD_ADDR_MIS_ALGN = 4,
    LOAD_ACC_FAULT = 5,
    STORE_AMO_ADDR_MIS_ALGN = 6,
    STORE_AMO_ACC_FAULT = 7,
    ECALL_U_MODE = 8,
    ECALL_S_MODE = 9,
    INS_PAGE_FAULT = 12,
    LOAD_PAGE_FAULT = 13,
    STORE_AMO_PAGE_FAULT = 15,
// USER mode doesn't manage interrupts
#ifdef SUPERVISOR
    // MSB is always 1
    SUPERVISOR_SOFTWARE_INTERRUPT = (int) 0x80000001,
    SUPERVISOR_TIMER_INTERRUPT = (int) 0x80000005,
    SUPERVISOR_EXTERNAL_INTERRUPT = (int) 0x80000009
#endif
} trap_code;

#define RESET_FAULT_VAL 0

void dispatch_trap(VCore *core, trap_code code, uint32_t faulting_val);

#endif
