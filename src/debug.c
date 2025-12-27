#include "debug.h"
#include "args.h"
#include "cpu.h"
#include "cthread.h"
#include "macros.h"
#include "memory.h"
#include "stubb_a_dub.h"
#include "threads_mgr2.h"
#include <assert.h>
#include <stdint.h>

// ASSUMING THE STUB IS SANITIZING INPUTS BASED ON SYS CONF PASSED

static uint64_t read_reg(uint32_t core_id, uint32_t reg_id) {
    const VCore *core = &threads_mgr.cthreads[core_id].core;
    return core->regs[reg_id];
}

static void write_reg(uint64_t input, uint32_t core_id, uint32_t reg_id) {
    VCore *core = &threads_mgr.cthreads[core_id].core;
    core->regs[reg_id] = (uint32_t) input;
}

static void read_mem(byte *output, uint64_t output_sz, uint64_t addr) {
    mem_rb_ptr_s((uint32_t) addr, output, output_sz);
}

static void write_mem(const byte *input, uint64_t input_sz, uint64_t addr) {
    mem_wb_ptr_s((uint32_t) addr, input, input_sz);
}

static const char *core_name(uint32_t core_id) {
    cthread_state state = cthread_get_hsm_state(&threads_mgr.cthreads[core_id]);

    switch (state) {
    case STATE_STARTED:
		return "[HSM] Started";
        break;
    case STATE_STOPPED:
		return "[HSM] Stopped";
        break;
    case STATE_SUSPENDED:
		return "[HSM] Suspended";
        break;
    case STATE_START_PENDING:
		return "[HSM] Start-pending";
        break;
    case STATE_STOP_PENDING:
		return "[HSM] Stop-pending";
        break;
    case STATE_SUSPEND_PENDING:
		return "[HSM] Suspend-pending";
        break;
    case STATE_RESUME_PENDING:
		return "[HSM] Resume-pending";
        break;
    default:
        return "";
        break;
    }
}

void run_debug(void) {
    stub_ret ret;

    Stub_Conf conf = {
        .sys_conf.arch = RV32,
        .sys_conf.regs_num = 33, // REG_NUMS,
        .sys_conf.pc_id = PC,
        .sys_conf.smp = ctx.cores, // core numbers
        .sys_conf.memory_size = ctx.memory_size,
        .sys_ops.core_name = core_name,
        .sys_ops.read_reg = read_reg,
        .sys_ops.write_reg = write_reg,
        .sys_ops.read_mem = read_mem,
        .sys_ops.write_mem = write_mem,
        .sys_ops.core_continue = threads_mgr_continue_core,
        .sys_ops.cores_continue = threads_mgr_continue_cores,
        .sys_ops.cores_halt = threads_mgr_halt_cores,
        .sys_ops.core_step = threads_mgr_step_core,
        .sys_ops.is_halted = threads_mgr_are_halted,
        .port = STUB_PORT,
        .buffers_size = STUB_BUFF_SIZE,
    };

    ret = sad_stub_init(&conf);
    if (ret != STUB_OK) {
        SV32_CRASH("Failed to initialize DEBUG Stub");
    }

    ret = sad_stub_handle_cmds();
    if (ret == STUB_CLOSED) {
        sad_stub_deinit();
        SV32_EXIT("Debugging STOPPED");
    }

    if (ret != STUB_OK) {
        sad_stub_deinit();
        SV32_CRASH("Error during DEBUGGING");
    }
}
