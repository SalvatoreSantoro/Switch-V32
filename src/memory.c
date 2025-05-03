#include "memory.h"
#include <stdint.h>
#include <stdio.h>

struct Memory __vmem = {0};

// BROKEN:FIX
// Write {Byte/Long/Word} data value in memory address "addr"
#define GEN_WS_FUN(sign_size, data_size)                                                                               \
    void mem_w##sign_size##_s(uint32_t addr, TYPE_##sign_size data, size_t sz) {                                       \
        memset(__vmem.m + addr, data, (sz) * (data_size));                                                             \
    }

#define GEN_W_FUN(sign_size, data_size)                                                                                \
    void mem_w##sign_size(uint32_t addr, TYPE_##sign_size data) {                                                      \
        memcpy(__vmem.m + addr, &data, (data_size));                                                                   \
    }

// Read {Byte/Long/Word} value in memory address "addr"
#define GEN_R_FUN(sign_size, data_size)                                                                                \
    TYPE_##sign_size mem_r##sign_size(uint32_t addr) {                                                                 \
        TYPE_##sign_size value;                                                                                        \
        memcpy(&value, __vmem.m + addr, (data_size));                                                                  \
        return value;                                                                                                  \
    }

// Write "sz" * {Bytes/Longs/Words} data pointed by "data" in memory address "addr"
#define GEN_WS_PTR_FUN(sign_size, data_size)                                                                           \
    void mem_w##sign_size##_ptr_s(uint32_t addr, const void *data, size_t sz) {                                        \
        memcpy(__vmem.m + addr, data, (sz) * (data_size));                                                             \
    }

// Read "sz" * {Bytes/Longs/Words} memory data pointed by "addr" in address "data"
#define GEN_R_PTR_FUN(sign_size, data_size)                                                                            \
    void mem_r##sign_size##_ptr_s(uint32_t addr, void *data, size_t sz) {                                              \
        memcpy(data, __vmem.m + addr, (sz) * (data_size));                                                             \
    }

GEN_WS_PTR_FUN(b, 1)
GEN_WS_PTR_FUN(h, 2)
GEN_WS_PTR_FUN(w, 4)

GEN_R_PTR_FUN(b, 1)
GEN_R_PTR_FUN(h, 2)
GEN_R_PTR_FUN(w, 4)

GEN_W_FUN(b, 1)
GEN_W_FUN(h, 2)
GEN_W_FUN(w, 4)

GEN_WS_FUN(b, 1)
GEN_WS_FUN(h, 2)
GEN_WS_FUN(w, 4)

GEN_R_FUN(b, 1)
GEN_R_FUN(h, 2)
GEN_R_FUN(w, 4)
