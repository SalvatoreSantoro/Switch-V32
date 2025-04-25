#ifndef _MEM32_H
#define _MEM32_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define TYPE_b uint8_t
#define TYPE_l uint16_t
#define TYPE_w uint32_t

#define PAGE_SIZE 4096

// 2^24
#define MEM_SIZE 16777216

struct Memory {
    uint8_t m[MEM_SIZE];
};

extern struct Memory __vmem __attribute__((aligned(PAGE_SIZE)));

// Write {Byte/Long/Word} data value in memory address "addr"
#define GEN_W_FUN_HDR(sign_size)                                                \
    void mem_w##sign_size##_s(uint32_t addr, TYPE_##sign_size data, size_t sz); \
    void mem_w##sign_size(uint32_t addr, TYPE_##sign_size data);

// Read {Byte/Long/Word} value in memory address "addr"
#define GEN_R_FUN_HDR(sign_size) \
    TYPE_##sign_size mem_r##sign_size(uint32_t addr);

// Write "sz" * {Bytes/Longs/Words} data pointed by "data" in memory address "addr"
#define GEN_W_PTR_FUN_HDR(sign_size)                                           \
    void mem_w##sign_size##_ptr_s(uint32_t addr, const void* data, size_t sz); \
    void mem_w##sign_size##_ptr(uint32_t addr, const void* data);

// Read "sz" * {Bytes/Longs/Words} memory data pointed by "addr" in address "data"
#define GEN_R_PTR_FUN_HDR(sign_size)                                     \
    void mem_r##sign_size##_ptr_s(uint32_t addr, void* data, size_t sz); \
    void mem_r##sign_size##_ptr(uint32_t addr, void* data);

GEN_W_PTR_FUN_HDR(b)
GEN_W_PTR_FUN_HDR(l)
GEN_W_PTR_FUN_HDR(w)

GEN_R_PTR_FUN_HDR(b)
GEN_R_PTR_FUN_HDR(l)
GEN_R_PTR_FUN_HDR(w)

GEN_W_FUN_HDR(b)
GEN_W_FUN_HDR(l)
GEN_W_FUN_HDR(w)

GEN_R_FUN_HDR(b)
GEN_R_FUN_HDR(l)
GEN_R_FUN_HDR(w)

#endif // !_MEM32_H
