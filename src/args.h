#ifndef _SV32_ARGS_H
#define _SV32_ARGS_H

#include "defs.h"
#include <stdbool.h>

typedef struct {
    char elf_name[PATH_MAX];
    const char *elf_stdin;
    const char *elf_stdout;
    const char *elf_stderr;
    const char *elf_args;
    unsigned int sdl_upscale;
    int cores;
	bool debug;
} Args_Context;

extern Args_Context ctx;

void ctx_init(int argc, char *argv[]);

#endif // !_RV32_ARGS_H
