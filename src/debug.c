#include "debug.h"
#include "args.h"
#include "cpu.h"
#include "macros.h"
#include "memory.h"
#include "stubb_a_dub.h"
#include "threads_mgr.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

// need to do more robust checks on memory and core_id
// in order to really return errors and not just crash with assert

static sys_err read_regs(byte *output, size_t output_sz, unsigned int core_id) {
    if (core_id >= ctx.cores)
        return CORE_OUT_OF_BOUNDS;

    if (!threads_mgr_is_halted(core_id, true))
        return CORE_RUNNING_ERR;

    const VCore *core = &GET_CORE(core_id);

    size_t regs_size = output_sz - 4;
    memcpy(output, &core->regs, regs_size);
    memcpy(output + regs_size, &core->pc, 4);

    return SYS_OK;
}

static sys_err write_regs(const byte *input, size_t input_sz, unsigned int core_id) {
    if (core_id >= ctx.cores)
        return CORE_OUT_OF_BOUNDS;

    if (!threads_mgr_is_halted(core_id, true))
        return CORE_RUNNING_ERR;

    size_t regs_size = input_sz - 4; // don't count PC
    VCore *core = &GET_CORE(core_id);
    // copy all regs
    memcpy(&core->regs, input, regs_size);
    // copy PC
    memcpy(&core->pc, input + regs_size, 4);

    return SYS_OK;
}

static sys_err read_mem(byte *output, size_t output_sz, uint32_t addr) {
    if (!VALID_ADDR(addr, output_sz))
        return MEMORY_OUT_OF_BOUNDS;

    mem_rb_ptr_s(addr, output, output_sz);
    return SYS_OK;
}

static sys_err write_mem(const byte *input, size_t input_sz, uint32_t addr) {
    if (!VALID_ADDR(addr, input_sz))
        return MEMORY_OUT_OF_BOUNDS;

    mem_wb_ptr_s(addr, input, input_sz);
    return SYS_OK;
}

static void core_continue(unsigned int core_id) {
    // do checks on core_idx
    threads_mgr_run_core(core_id);
}

static void cores_continue(void) {
    // do checks on core_idx
    threads_mgr_run_all();
}

static void cores_halt(void) {
    // do checks on core_idx
    threads_mgr_halt_all();
}

static sys_err core_step(unsigned int core_id) {
    // do checks on core_idx
    if (threads_mgr_is_halted(core_id, true)) {
        threads_mgr_step_core(core_id);
        return SYS_OK;
    }
    return CORE_RUNNING_ERR;
}

static bool core_is_halted(unsigned core_id) {
    // do checks on core_idx
    return threads_mgr_is_halted(core_id, false);
}

void *debug_thread_fun(void *args) {
    stub_ret ret;

    Stub_Conf conf = {
        .sys_conf.arch = RV32,
        .sys_conf.regs_num = REG_NUMS + 1, // PC isn't counted in regs
        .sys_conf.smp = ctx.cores,         // core numbers
        .sys_ops.read_regs = read_regs,
        .sys_ops.write_regs = write_regs,
        .sys_ops.read_mem = read_mem,
        .sys_ops.write_mem = write_mem,
        .sys_ops.core_continue = core_continue,
        .sys_ops.cores_continue = cores_continue,
        .sys_ops.cores_halt = cores_halt,
        .sys_ops.core_step = core_step,
        .sys_ops.is_halted = core_is_halted,
        .port = STUB_PORT,
        .buffers_size = STUB_BUFF_SIZE,
        .socket_io_size = STUB_READ_SIZE,
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

    return NULL;
}
