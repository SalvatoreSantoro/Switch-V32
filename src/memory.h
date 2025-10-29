#ifndef _SV32_MEM_H
#define _SV32_MEM_H

#include <stdint.h>
#include <string.h>
#include "defs.h"

#define TYPE_b uint8_t
#define TYPE_h uint16_t
#define TYPE_w uint32_t

struct Memory {
    uint8_t m[MEM_SIZE];
};

// map address in the virtualized code to the correct memory address
#define MAP_ADDR(addr) ((void*) (g_mem.m + addr))

extern struct Memory g_mem __attribute__((aligned(PAGE_SIZE)));

void mem_wb_s(uint32_t addr, uint8_t data, size_t data_size);

// Write {Byte/Long/Word} data value in memory address "addr"
#define GEN_W_FUN_HDR(sign_size) void mem_w##sign_size(uint32_t addr, TYPE_##sign_size data);

// Read {Byte/Long/Word} value in memory address "addr"
#define GEN_R_FUN_HDR(sign_size) TYPE_##sign_size mem_r##sign_size(uint32_t addr);

// Write "sz" * {Bytes/Longs/Words} data pointed by "data" in memory address "addr"
#define GEN_W_PTR_FUN_HDR(sign_size)                                                                                   \
    void mem_w##sign_size##_ptr_s(uint32_t addr, const void *data, size_t sz);                                         \
    void mem_w##sign_size##_ptr(uint32_t addr, const void *data);

// Read "sz" * {Bytes/Longs/Words} memory data pointed by "addr" in address "data"
#define GEN_R_PTR_FUN_HDR(sign_size)                                                                                   \
    void mem_r##sign_size##_ptr_s(uint32_t addr, void *data, size_t sz);                                               \
    void mem_r##sign_size##_ptr(uint32_t addr, void *data);

GEN_W_PTR_FUN_HDR(b)
GEN_W_PTR_FUN_HDR(h)
GEN_W_PTR_FUN_HDR(w)

GEN_R_PTR_FUN_HDR(b)
GEN_R_PTR_FUN_HDR(h)
GEN_R_PTR_FUN_HDR(w)

GEN_W_FUN_HDR(b)
GEN_W_FUN_HDR(h)
GEN_W_FUN_HDR(w)

GEN_R_FUN_HDR(b)
GEN_R_FUN_HDR(h)
GEN_R_FUN_HDR(w)

#define mem_rb_ptr(addr, data) mem_rb_ptr_s(addr, data, 1)
#define mem_rh_ptr(addr, data) mem_rh_ptr_s(addr, data, 1)
#define mem_rw_ptr(addr, data) mem_rw_ptr_s(addr, data, 1)

#define mem_wb_ptr(addr, data) mem_wb_ptr_s(addr, data, 1)
#define mem_wh_ptr(addr, data) mem_wh_ptr_s(addr, data, 1)
#define mem_ww_ptr(addr, data) mem_ww_ptr_s(addr, data, 1)

#endif // !_MEM32_H
