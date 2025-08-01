#include "buffer.h"
#include "callback.h"
#include "cpu.h"
#include "memory.h"
#include "stub.h"
#include <stdint.h>
#include <stdio.h>

DEFINE_CALLBACK(regs_read_callback, &core, READ_REGS_CBK) {
    // append converting to little endian
    gdb_buff_append_bytes(handler_data->output, (unsigned char *) user_data->regs, REG_NUMS * sizeof(uint32_t));
    gdb_buff_append_bytes(handler_data->output, (unsigned char *) &user_data->pc, sizeof(uint32_t));
}

DEFINE_CALLBACK(regs_write_callback, &core, WRITE_REGS_CBK) {
    int i = 0;
    for (i = 0; i < REG_NUMS; i++)
        user_data->regs[i] = handler_data->regs[i];

    user_data->pc = handler_data->regs[i];
}

DEFINE_CALLBACK(mem_read_callback, &g_mem, READ_MEM_CBK) {
    unsigned char data[handler_data->length];
    mem_rb_ptr_s(handler_data->addr, data, handler_data->length);
    gdb_buff_append_bytes(handler_data->output, data, handler_data->length);
}

DEFINE_CALLBACK(mem_write_callback, &g_mem, WRITE_MEM_CBK) {
    mem_wb_ptr_s(handler_data->addr, handler_data->data, handler_data->length);
}
