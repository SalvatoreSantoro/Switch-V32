#ifndef _RV32_ARGS_H
#define _RV32_ARGS_H

#include "memory.h"
#include <stdio.h>

typedef struct {
    const char *elf_name;
    const char *elf_stdin;
    const char *elf_stdout;
    const char *elf_stderr;
    // the layout is |STACK_BASE|ARGC|ARGV[0]|ARGV[1]|...|ARGV[ARGC-1]|NULL|STR0|STR1|...|STR_ARGC-1|
    unsigned char elf_args[PAGE_SIZE];
    int sdl_upscale;
} Args_Context;

extern Args_Context ctx;

void ctx_init(int argc, char *argv[]);

#endif // !_RV32_ARGS_H
