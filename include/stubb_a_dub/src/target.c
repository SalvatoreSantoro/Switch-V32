#include "sad_gdb_internal.h"
#include "stubb_a_dub.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Sys_Conf sys_conf_g;
extern Target_Conf target_conf_g;
extern Sys_Ops sys_ops_g;

stub_ret sad_target_init(void) {
    switch (sys_conf_g.arch) {
    case RV32:
        // 32 bit architecture
        target_conf_g.breakpoint_val = 0x00100073;
        target_conf_g.reg_bytes = 4;
        target_conf_g.reg_str_bytes = 8;
        target_conf_g.regs_bytes = target_conf_g.reg_bytes * sys_conf_g.regs_num;
        target_conf_g.regs_str_bytes = target_conf_g.reg_str_bytes * sys_conf_g.regs_num;
        break;
    default:
        return STUB_UNEXPECTED;
    }
    return STUB_OK;
}

void sad_target_read_regs(byte *regs, uint32_t core_id) {
    uint64_t reg;
    byte *reg_byte;
    uint32_t tot = 0;

    for (uint32_t reg_id = 0; reg_id < sys_conf_g.regs_num; reg_id++) {
        reg = sys_ops_g.read_reg(core_id, reg_id);

        reg_byte = (byte *) &reg;

        // read it like a stream of bytes to be 32/64 bit compatible
        for (uint32_t byte_idx = 0; byte_idx < target_conf_g.reg_bytes; byte_idx++) {
            *(regs + tot) = reg_byte[byte_idx];
            tot++;
        }
    }
}

void sad_target_write_regs(const byte *regs, uint32_t core_id) {
    uint64_t reg;

    byte *reg_bytes = (byte *) &reg;

    for (uint32_t reg_id = 0; reg_id < sys_conf_g.regs_num; reg_id++) {
        memcpy(reg_bytes, regs + (reg_id * target_conf_g.reg_bytes), target_conf_g.reg_bytes);
        sys_ops_g.write_reg(reg, core_id, reg_id);
    }
}
