#include "debug.h"
#include "args.h"
#include "cpu.h"
#include "macros.h"
#include "memory.h"
#include "sad_gdb.h"
#include "threads_mgr.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

extern Threads_Mgr threads_mgr;

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
        .sys_ops.core_continue = threads_mgr_continue_core,
        .sys_ops.cores_continue = threads_mgr_continue_all,
        .sys_ops.cores_halt = threads_mgr_halt_all,
        .sys_ops.core_step = threads_mgr_step_core,
        .sys_ops.is_halted = threads_mgr_is_halted,
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

// core_id unused for now
void read_regs(byte *output, size_t output_sz, int core_id) {
    assert(core_id < ctx.cores);
    VCore *core = &GET_CORE(core_id);

    size_t regs_size = output_sz - 4; // don't count PC
    // copy all regs
    memcpy(output, &core->regs, regs_size);
    // copy PC
    memcpy(output + regs_size, &core->pc, 4);
}

// core_id unused for now
void write_regs(const byte *input, size_t input_sz, int core_id) {
    assert(core_id < ctx.cores);
    size_t regs_size = input_sz - 4; // don't count PC
    VCore *core = &GET_CORE(core_id);
    // copy all regs
    memcpy(&core->regs, input, regs_size);
    // copy PC
    memcpy(&core->pc, input + regs_size, 4);
}

void read_mem(byte *output, size_t output_sz, uint32_t addr) {
    mem_rb_ptr_s(addr, output, output_sz);
}

void write_mem(const byte *input, size_t input_sz, uint32_t addr) {
    mem_wb_ptr_s(addr, input, input_sz);
}
