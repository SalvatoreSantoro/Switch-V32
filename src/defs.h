#ifndef _SV32_DEFS_H
#define _SV32_DEFS_H

#include <limits.h>

#ifndef PATH_MAX
    #define PATH_MAX 4096
#endif

#ifdef SUPERVISOR 
#define SUPERVISOR_IS_SET 1
#else
#define SUPERVISOR_IS_SET 0
#endif

// 2^28
#define MEM_SIZE 268435456
#define PAGE_SIZE 4096
#define DCACHE_LINE_SIZE 64

// end of memory, leaving 1 page under empty (argc, argv, env)
#define STACK_BASE (MEM_SIZE - PAGE_SIZE)
#define BRK_LIMIT (MEM_SIZE >> 4)

// GDB STUB
#define STUB_PORT 1234
#define STUB_BUFF_SIZE 128
#define STUB_READ_SIZE 256

#endif
