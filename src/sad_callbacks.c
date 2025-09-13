#include "buffer.h"
#include "callback.h"
#include "cpu.h"
#include "defs.h"
#include "memory.h"
#include "stub.h"
#include <stdint.h>
#include <stdio.h>

DEFINE_CALLBACK(regs_read_callback, READ_REGS_CBK) {
    // append converting to little endian
    sad_buff_append(handler_data->output, (byte *) &core.regs, REG_NUMS * 4);
    sad_buff_append(handler_data->output, (byte *) &core.pc, 4);
}

DEFINE_CALLBACK(regs_write_callback, WRITE_REGS_CBK) {
    int i = 0;
    for (i = 0; i < REG_NUMS; i++)
        core.regs[i] = handler_data->regs[i];

    core.pc = handler_data->regs[i];
}

DEFINE_CALLBACK(mem_read_callback, READ_MEM_CBK) {
    byte data[handler_data->length];
    mem_rb_ptr_s(handler_data->addr, data, handler_data->length);
    sad_buff_append(handler_data->output, data, handler_data->length);
}

DEFINE_CALLBACK(mem_write_callback, WRITE_MEM_CBK) {
    mem_wb_ptr_s(handler_data->addr, handler_data->data, handler_data->length);
}
