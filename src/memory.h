#ifndef SV32_MEM_H
#define SV32_MEM_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

extern uint8_t *vmem;

void mem_init(void);

void mem_deinit(void);

#define MAP_ADDR(addr)        ((void *) (vmem + addr))

// ASSUMING ALWAYS ALIGNED ACCESS

static inline uint8_t align_mem_rb(uintptr_t addr) {
    return *((uint8_t *) MAP_ADDR(addr));
}

static inline uint16_t align_mem_rh(uintptr_t addr) {
    return *((uint16_t *) MAP_ADDR(addr));
}

static inline uint32_t align_mem_rw(uintptr_t addr) {
    return *((uint32_t *) MAP_ADDR(addr));
}

static inline void align_mem_wb(uintptr_t addr, uint8_t data) {
    *((uint8_t *) MAP_ADDR(addr)) = data;
}

static inline void align_mem_wh(uintptr_t addr, uint16_t data) {
    *((uint16_t *) MAP_ADDR(addr)) = data;
}

static inline void align_mem_ww(uintptr_t addr, uint32_t data) {
    *((uint32_t *) MAP_ADDR(addr)) = data;
}

static inline void mem_rb_ptr_s(uintptr_t addr, void *output, size_t output_sz) {
    memcpy(output, MAP_ADDR(addr), output_sz);
}

static inline void mem_wb_ptr_s(uintptr_t addr, const void *input, size_t input_sz) {
    memcpy(MAP_ADDR(addr), input, input_sz);
}

static inline void mem_wb_s(uintptr_t addr, uint8_t input, size_t write_sz) {
    memset(MAP_ADDR(addr), input, write_sz);
}
#endif // !_MEM32_H
