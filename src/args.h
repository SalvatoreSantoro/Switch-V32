#ifndef SV32_ARGS_H
#define SV32_ARGS_H

#include "defs.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    char elf_name[PATH_MAX];
    const char *elf_stdin;
    const char *elf_stdout;
    const char *elf_stderr;
    const char *elf_args;
    unsigned int sdl_upscale;
    unsigned int cores;
    bool debug;
    bool binary;
    size_t path_max;
	//uint64 because to allow a 4GB of maximum size we need to
	//represent UINT32_MAX + 1
    uint64_t memory_size;
    uint32_t stack_base;
#ifdef USER
    uint32_t brk_limit;
#endif
} Args_Context;

extern Args_Context ctx;

void ctx_init(int argc, char *argv[]);

#endif // !_RV32_ARGS_H
