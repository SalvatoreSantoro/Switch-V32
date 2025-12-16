#include "sbi.h"
#include "args.h"
#include "cpu.h"
#include "csr.h"
#include "threads_mgr2.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define BASE_EID (0x10)
#define HSM_EID  (0x48534D)
// in order: HSM
#define EXTS_NUM 2

static uint32_t supported_ext[EXTS_NUM] = {BASE_EID, HSM_EID};

typedef enum {
    SBI_SUCCESS = 0,
    SBI_ERR_FAILED = -1,
    SBI_ERR_NOT_SUPPORTED = -2,
    SBI_ERR_INVALID_PARAM = -3,
    SBI_ERR_DENIED = -4,
    SBI_ERR_INVALID_ADDRESS = -5,
    SBI_ERR_ALREADY_AVAILABLE = -6,
    SBI_ERR_ALREADY_STARTED = -7,
    SBI_ERR_ALREADY_STOPPED = -8,
    SBI_ERR_NO_SHMEM = -9,
    SBI_ERR_INVALID_STATE = -10,
    SBI_ERR_BAD_RANGE = -11,
    SBI_ERR_TIMEOUT = -12,
    SBI_ERR_IO = -13,
    SBI_ERR_DENIED_LOCKED = -14,
} sbi_errors;

#define BASE_FID_0 (0x0)
#define BASE_FID_1 (0x1)
#define BASE_FID_2 (0x2)
#define BASE_FID_3 (0x3)
#define BASE_FID_4 (0x4)
#define BASE_FID_5 (0x5)
#define BASE_FID_6 (0x6)

#define HSM_FID_0 (0x0)
#define HSM_FID_1 (0x1)
#define HSM_FID_2 (0x2)
#define HSM_FID_3 (0x3)

#define HSM_STATE_STARTED         (0x0)
#define HSM_STATE_STOPPED         (0x1)
#define HSM_STATE_START_PENDING   (0x2)
#define HSM_STATE_STOP_PENDING    (0x3)
#define HSM_STATE_SUSPEND         (0x4)
#define HSM_STATE_SUSPEND_PENDING (0x5)
#define HSM_STATE_RESUME_PENDING  (0x6)

#define HSM_RETENTIVE_SUSPEND     (0x00000000)
#define HSM_NON_RETENTIVE_SUSPEND (0x80000000)

static void base_ext(VCore *core) {
    bool found = false;
    switch (core->regs[A6]) {
    // get SBI specification version
    case BASE_FID_0:
        // The minor number of
        // the SBI specification is encoded in the low 24 bits,
        // with the major number encoded in the next 7 bits. Bit 31
        // must be 0 and is reserved for future expansion
        // Specifying version 3.0
        core->regs[A1] = 0x03000000;
        break;
    case BASE_FID_3:
        for (int i = 0; i < EXTS_NUM; i++) {
            if (core->regs[A0] == supported_ext[i]) {
                found = true;
                break;
            }
        }
        if (found)
            core->regs[A0] = 1;
        else
            core->regs[A0] = 0;
        break;
    // get SBI implementation ID
    case BASE_FID_1:
    // get SBI implementation version
    case BASE_FID_2:
    // get machine vendor ID (mvendorid)
    case BASE_FID_4:
    // get machine architecture (marchid)
    case BASE_FID_5:
    // get machine implementation (mimpid)
    case BASE_FID_6:
        core->regs[A1] = 0;
        break;
    default:
        core->regs[A0] = (uint32_t) SBI_ERR_NOT_SUPPORTED;
        return;
    }
    // Always success
    core->regs[A0] = SBI_SUCCESS;
}

// we ignore pending and suspend states, a core can only be started
// or suspended, we just synchronously check the core's state
static void hsm_ext(VCore *core) {
    Core_State state;
    uint32_t hart_id;
    uint32_t start_addr;
    uint32_t opaque;
    uint32_t suspend_type;

    switch (core->regs[A6]) {
    // Hart Start
    case HSM_FID_0:
        hart_id = core->regs[A0];
        start_addr = core->regs[A1];
        opaque = core->regs[A2];

        if (hart_id >= ctx.cores) {
            core->regs[A0] = (uint32_t) SBI_ERR_INVALID_PARAM;
            return;
        }

        if (start_addr >= ctx.memory_size) {
            core->regs[A0] = (uint32_t) SBI_ERR_INVALID_ADDRESS;
            return;
        }

        state = threads_mgr_get_state(hart_id);
        if (state != STATE_STOPPED) {
            core->regs[A0] = (uint32_t) SBI_ERR_ALREADY_AVAILABLE;
            return;
        }
        threads_mgr_init_core(hart_id, opaque, start_addr);
        threads_mgr_signal_halt(hart_id, true);
        // if we're not debugging just start the core, otherwise GDB will start it
        // and we're just setting it on to be continued by gdb
        if (!ctx.debug)
            threads_mgr_run_core(hart_id);
        break;

    // Hart Stop
    case HSM_FID_1:
        // check if Supervisor-Mode interrupt are enabled
        if (SSTATUS_SIE(core->sstatus)) {
            core->regs[A0] = (uint32_t) SBI_ERR_FAILED;
            return;
        }
        // halt yourself
        threads_mgr_signal_stop(core->core_idx, false);
        break;

    case HSM_FID_2:
        hart_id = core->regs[A0];
        if (hart_id >= ctx.cores) {
            core->regs[A0] = (uint32_t) SBI_ERR_INVALID_PARAM;
            return;
        }
        state = threads_mgr_get_state(hart_id);

        switch (state) {
        // when using gdb halted is like running but the core isn't
        // effectively running, it's stopped waiting for continue signals
        case STATE_HALTED:
        case STATE_RUNNING:
            core->regs[A1] = HSM_STATE_STARTED;
            break;

        case STATE_STOPPED:
            core->regs[A1] = HSM_STATE_STOPPED;
            break;

        case STATE_START_PENDING:
            core->regs[A1] = HSM_STATE_START_PENDING;
            break;

        case STATE_STOP_PENDING:
            core->regs[A1] = HSM_STATE_STOP_PENDING;
            break;

        case STATE_SUSPEND_PENDING:
            core->regs[A1] = HSM_STATE_SUSPEND_PENDING;
            break;

        case STATE_RESUME_PENDING:
            core->regs[A1] = HSM_STATE_RESUME_PENDING;
            break;

        case STATE_SUSPENDED:
            core->regs[A1] = HSM_STATE_SUSPEND;
            break;

        default:
            assert(0 && "Some suspended state aren't implemented");
            break;
        }

    case HSM_FID_3:
        suspend_type = core->regs[A0];
        start_addr = core->regs[A1];
        opaque = core->regs[A2];

        if (start_addr >= ctx.memory_size) {
            core->regs[A0] = (uint32_t) SBI_ERR_INVALID_ADDRESS;
            return;
        }

        // keep the state and return (the execution will stop due
        // to the check in the VCORE while loop)

        if (suspend_type == HSM_RETENTIVE_SUSPEND) {
            break;
        } else if (suspend_type == HSM_NON_RETENTIVE_SUSPEND) {
            core->regs[A0] = core->core_idx;
            core->regs[A1] = opaque;
            core->pc = start_addr;
        } else {
            core->regs[A0] = (uint32_t) SBI_ERR_INVALID_PARAM;
            return;
        }

        // finally suspend
        assert(0 && "Some suspended state aren't implemented");
        break;

    default:
        core->regs[A0] = (uint32_t) SBI_ERR_NOT_SUPPORTED;
        return;
    }

    core->regs[A0] = SBI_SUCCESS;
}

void emu_sbi_call(VCore *core) {
    // switch on extension
    switch (core->regs[A7]) {

    case BASE_EID:
        base_ext(core);
        break;

    case HSM_EID:
        hsm_ext(core);
        break;

    default:
        core->regs[A0] = (uint32_t) SBI_ERR_NOT_SUPPORTED;
    }
}
