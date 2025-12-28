#include "sbi.h"
#include "args.h"
#include "cpu.h"
#include "cthread.h"
#include "threads_mgr2.h"
#include <assert.h>
#include <stdint.h>

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
    cthread_state state;
    uint32_t hart_id = core->regs[A0];
    uint32_t start_addr = core->regs[A1];
    uint32_t opaque = core->regs[A2];
    uint32_t suspend_type = core->regs[A0];
    VCore_Init init = {0};

    switch (core->regs[A6]) {
    // Hart Start
    case HSM_FID_0:
        if (hart_id >= ctx.cores) {
            core->regs[A0] = (uint32_t) SBI_ERR_INVALID_PARAM;
            return;
        }

        if (start_addr >= ctx.memory_size) {
            core->regs[A0] = (uint32_t) SBI_ERR_INVALID_ADDRESS;
            return;
        }

        state = cthread_get_hsm_state(&threads_mgr.cthreads[hart_id]);
        if ((state == STATE_START_PENDING) || (state == STATE_STARTED)) {
            core->regs[A0] = (uint32_t) SBI_ERR_ALREADY_AVAILABLE;
            return;
        }

        init.pc = start_addr;
        init.id = hart_id;
        init.opaque = opaque;

        vcore_init(&threads_mgr.cthreads[hart_id].core, &init);
        cthread_signal_start(&threads_mgr.cthreads[hart_id]);
        break;

    // Hart Stop
    case HSM_FID_1:
        // stop yourself
        cthread_signal_stop(&threads_mgr.cthreads[core->core_idx]);
        break;

    case HSM_FID_2:
        if (hart_id >= ctx.cores) {
            core->regs[A0] = (uint32_t) SBI_ERR_INVALID_PARAM;
            return;
        }
        // state values already compatible
        core->regs[A1] = cthread_get_hsm_state(&threads_mgr.cthreads[hart_id]);
        break;

    case HSM_FID_3:
        if (start_addr >= ctx.memory_size) {
            core->regs[A0] = (uint32_t) SBI_ERR_INVALID_ADDRESS;
            return;
        }
        // keep the state and return (the execution will stop due
        // to the check in the VCORE while loop)

        if (suspend_type == HSM_RETENTIVE_SUSPEND) {
            cthread_signal_suspend(&threads_mgr.cthreads[hart_id]);
            break;
        }
        if (suspend_type == HSM_NON_RETENTIVE_SUSPEND) {
            init.pc = start_addr;
            init.id = hart_id;
            init.opaque = opaque;

            vcore_init(&threads_mgr.cthreads[hart_id].core, &init);
            break;
        }
        core->regs[A0] = (uint32_t) SBI_ERR_INVALID_PARAM;
        return;

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
