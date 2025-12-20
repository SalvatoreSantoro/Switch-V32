#include "debug.h"
#include "args.h"
#include "cpu.h"
#include "macros.h"
#include "memory.h"
#include "stubb_a_dub.h"
#include "threads_mgr2.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

// need to do more robust checks on memory and core_id
// in order to really return errors and not just crash with assert

static sys_err read_regs(byte *output, size_t output_sz, unsigned int core_id) {
    if (core_id >= ctx.cores)
        return CORE_OUT_OF_BOUNDS;

    if (!threads_mgr_are_halted())
        return CORE_RUNNING_ERR;

    const VCore *core = &threads_mgr.cthreads[core_id].core;

    size_t regs_size = output_sz - 4;
    memcpy(output, &core->regs, regs_size);
    memcpy(output + regs_size, &core->pc, 4);

    return SYS_OK;
}

static sys_err write_regs(const byte *input, size_t input_sz, unsigned int core_id) {
    if (core_id >= ctx.cores)
        return CORE_OUT_OF_BOUNDS;

    if (!threads_mgr_are_halted())
        return CORE_RUNNING_ERR;

    size_t regs_size = input_sz - 4; // don't count PC

    VCore *core = &threads_mgr.cthreads[core_id].core;
    // copy all regs
    memcpy(&core->regs, input, regs_size);
    // copy PC
    memcpy(&core->pc, input + regs_size, 4);
    return SYS_OK;
}

static sys_err read_mem(byte *output, size_t output_sz, uint32_t addr) {
    if (!VALID_ADDR(addr, output_sz))
        return MEMORY_OUT_OF_BOUNDS;

    for (unsigned int core_id = 0; core_id < ctx.cores; core_id++) {
        if (!threads_mgr_are_halted()) {
            return CORE_RUNNING_ERR;
        }
    }

    mem_rb_ptr_s(addr, output, output_sz);
    return SYS_OK;
}

static sys_err write_mem(const byte *input, size_t input_sz, uint32_t addr) {
    if (!VALID_ADDR(addr, input_sz))
        return MEMORY_OUT_OF_BOUNDS;

    for (unsigned int core_id = 0; core_id < ctx.cores; core_id++) {
        if (!threads_mgr_are_halted()) {
            return CORE_RUNNING_ERR;
        }
    }

    mem_wb_ptr_s(addr, input, input_sz);
    return SYS_OK;
}

static sys_err core_continue(unsigned int core_id) {
    if (core_id >= ctx.cores)
        return CORE_OUT_OF_BOUNDS;
    threads_mgr_continue_core(core_id);

    return SYS_OK;
}

static void cores_continue(void) {
    threads_mgr_continue_cores();
}

static void cores_halt(void) {
    threads_mgr_halt_cores();
}

static sys_err core_step(unsigned int core_id) {
    if (core_id >= ctx.cores)
        return CORE_OUT_OF_BOUNDS;

    if (threads_mgr_are_halted()) {
        threads_mgr_step_core(core_id);
        return SYS_OK;
    }

    return CORE_RUNNING_ERR;
}

void run_debug(void) {
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
        .sys_ops.is_halted = threads_mgr_are_halted,
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
}
