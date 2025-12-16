#include "memory.h"
#include "args.h"
#include "defs.h"
#include "macros.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

uint8_t *vmem;

void mem_init(void) {
    vmem = mmap(NULL, ctx.memory_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (vmem == MAP_FAILED) {
        printf("%ld\n", ctx.memory_size);
        perror("MMAP");
        SV32_CRASH("MMAP CRITIAL ERROR");
    }

    // check page alignment
    assert(((uintptr_t) vmem & (PAGE_SIZE - 1)) == 0);

#ifdef SUPERVISOR
    /* bootstrap_code:*/
    /* .section .text */
    /*     .org 0x0 */
    /* .global _start */
    /* _start: */
    /*     j   loop_b */
    /**/
    /* loop_b: */
    /*     j   _start */
    static const uint32_t bootstrap_code[3] = {0x0040006f, 0xffdff06f};
    memcpy(vmem, bootstrap_code, sizeof(bootstrap_code));
#endif
}

void mem_deinit(void) {
    if (munmap(vmem, ctx.memory_size) == -1) {
        perror("MUNMAP");
        SV32_CRASH("MUNMAP CRITIAL ERROR");
    }
}
