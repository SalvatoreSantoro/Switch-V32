#include "memory.h"
#include "args.h"
#include "defs.h"
#include "macros.h"
#include <assert.h>
#include <stdio.h>
#include <sys/mman.h>

uint8_t *vmem;

void mem_init(void) {
    vmem = mmap(NULL, ctx.memory_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (vmem == MAP_FAILED){
		printf("%ld\n", ctx.memory_size);
		perror("MMAP");
		SV32_CRASH("MMAP CRITIAL ERROR");
	}

    // check page alignment
    assert(((intptr_t) vmem & (PAGE_SIZE - 1)) == 0);
}

void mem_deinit(void) {
    if (munmap(vmem, ctx.memory_size) == -1){
		perror("MUNMAP");
		SV32_CRASH("MUNMAP CRITIAL ERROR");
	}
}
