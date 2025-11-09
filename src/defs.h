#ifndef SV32_DEFS_H
#define SV32_DEFS_H

#include <linux/limits.h>

#ifdef SUPERVISOR
    #define SUPERVISOR_IS_SET 1
#else
    #define SUPERVISOR_IS_SET 0
#endif

#define PAGE_SIZE        4096u
#define DCACHE_LINE_SIZE 64u

// LIMITS
#define MAX_UPSCALE     8u
#define MAX_MEM_SIZE_MB 4096u
#define MAX_NPROCS      4u

#ifndef PATH_MAX
    #define PATH_MAX 4096
#endif

// DEFAULTS

// MEMORY

// 2^30
#define DEF_MEM_SIZE 1073741824u
// end of memory, leaving 1 page under empty (argc, argv, env)

#define DEF_STACK_BASE (DEF_MEM_SIZE - PAGE_SIZE)
#ifdef USER
    #define DEF_BRK_LIMIT (DEF_MEM_SIZE >> 4)
#endif

// GDB STUB
#define STUB_PORT      1234u
#define STUB_BUFF_SIZE 128u
#define STUB_READ_SIZE 256u

#endif
