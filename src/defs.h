#ifndef _RISCV32_DEFS_H
#define _RISCV32_DEFS_H

#include <limits.h>

#ifndef PATH_MAX
    #define PATH_MAX 4096
#endif

// 2^28
//#define MEM_SIZE 268435456
//#define PAGE_SIZE 4096

// end of memory, leaving 1 page under empty (argc, argv, env)
//#define STACK_BASE (MEM_SIZE - PAGE_SIZE)
//#define BRK_LIMIT ((uintptr_t)g_mem.m + (MEM_SIZE >> 4))

#endif
