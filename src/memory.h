#ifndef SV32_MEM_H
#define SV32_MEM_H

#include "args.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern uint8_t *vmem;

void mem_init(void);

void mem_deinit(void);

#define MAP_ADDR(addr) ((void *) (vmem + addr))

#define VALID_ADDR(addr, io_sz) (((uintptr_t) addr + io_sz) < ctx.memory_size)

// ASSUMING ALWAYS ALIGNED ACCESS

static inline uint8_t mem_rb(uint32_t addr) {
    return *((uint8_t *) MAP_ADDR(addr));
}

static inline uint16_t mem_rh(uint32_t addr) {
    return *((uint16_t *) MAP_ADDR(addr));
}

static inline uint32_t mem_rw(uint32_t addr) {
    return *((uint32_t *) MAP_ADDR(addr));
}

static inline void mem_wb(uint32_t addr, uint8_t data) {
    *((uint8_t *) MAP_ADDR(addr)) = data;
}

static inline void mem_wh(uint32_t addr, uint16_t data) {
    *((uint16_t *) MAP_ADDR(addr)) = data;
}

static inline void mem_ww(uint32_t addr, uint32_t data) {
    *((uint32_t *) MAP_ADDR(addr)) = data;
}

static inline uint32_t mem_aadd(uint32_t addr, uint32_t val, int memorder) {
    return __atomic_fetch_add((uint32_t *) MAP_ADDR(addr), val, memorder);
}

static inline uint32_t mem_aand(uint32_t addr, uint32_t val, int memorder) {
    return __atomic_fetch_and((uint32_t *) MAP_ADDR(addr), val, memorder);
}

static inline uint32_t mem_axor(uint32_t addr, uint32_t val, int memorder) {
    return __atomic_fetch_xor((uint32_t *) MAP_ADDR(addr), val, memorder);
}

static inline uint32_t mem_aor(uint32_t addr, uint32_t val, int memorder) {
    return __atomic_fetch_or((uint32_t *) MAP_ADDR(addr), val, memorder);
}

static inline uint32_t mem_anand(uint32_t addr, uint32_t val, int memorder) {
    return __atomic_fetch_nand((uint32_t *) MAP_ADDR(addr), val, memorder);
}

static inline uint32_t mem_aswp(uint32_t addr, uint32_t val, int memorder) {
    return __atomic_exchange_n((uint32_t *) MAP_ADDR(addr), val, memorder);
}

static inline uint32_t mem_aread(uint32_t addr, int memorder) {
    return __atomic_load_n((uint32_t *) MAP_ADDR(addr), memorder);
}

static inline void mem_cas(uint32_t addr, uint32_t val, uint32_t expected, int memorder) {
    while (
        !__atomic_compare_exchange_n((uint32_t *) MAP_ADDR(addr), &expected, val, false, memorder, __ATOMIC_RELAXED)) {
    }
}

static inline uint32_t mem_amaxu(uint32_t addr, uint32_t val, int memorder) {
    uint32_t old_val = mem_aread(addr, __ATOMIC_RELAXED);

    if (old_val > val)
        return old_val;

    uint32_t expected = old_val;

    mem_cas(addr, val, expected, memorder);

    return old_val;
}

static inline uint32_t mem_aminu(uint32_t addr, uint32_t val, int memorder) {
    uint32_t old_val = mem_aread(addr, __ATOMIC_RELAXED);
    if (old_val < val)
        return old_val;

    uint32_t expected = old_val;

    mem_cas(addr, val, expected, memorder);

    return old_val;
}

static inline int32_t mem_amax(uint32_t addr, int32_t val, int memorder) {
    int32_t old_val = (int32_t) mem_aread(addr, __ATOMIC_RELAXED);
    if (old_val > val)
        return old_val;

    uint32_t expected = (uint32_t) old_val;

    mem_cas(addr, (uint32_t) val, expected, memorder);

    return old_val;
}

static inline int32_t mem_amin(uint32_t addr, int32_t val, int memorder) {
    int32_t old_val = (int32_t) mem_aread(addr, __ATOMIC_RELAXED);
    if (old_val < val)
        return old_val;

    uint32_t expected = (uint32_t) old_val;

    mem_cas(addr, (uint32_t) val, expected, memorder);

    return old_val;
}

static inline void mem_rb_ptr_s(uint32_t addr, void *output, size_t output_sz) {
    memcpy(output, MAP_ADDR(addr), output_sz);
}

static inline void mem_wb_ptr_s(uint32_t addr, const void *input, size_t input_sz) {
    memcpy(MAP_ADDR(addr), input, input_sz);
}

static inline void mem_wb_s(uint32_t addr, uint8_t input, size_t write_sz) {
    memset(MAP_ADDR(addr), input, write_sz);
}
#endif // !_MEM32_H
