#include "trap.h"
#include "emu.h"
#include "threads_mgr2.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void dispatch_trap(VCore *core, trap_code code, uint32_t faulting_val) {
    switch (code) {
    case ILL_INS:
        fprintf(stderr, "BADOPCODE at %x\n", core->pc);
        exit(EXIT_FAILURE);
        break;
    case BRKPT:
        threads_mgr_halt_all();
        break;
    case ECALL_U_MODE:
        emu_system_call(core);
        break;
    case INS_PAGE_FAULT:
        break;
    case LOAD_PAGE_FAULT:
        break;
    case STORE_AMO_PAGE_FAULT:
        break;
    default:
        assert(0 && "wrong exception code");
        break;
    }
}
