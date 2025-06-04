#include "args.h"
#include <bits/getopt_core.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// clang-format off
Args_Context ctx = {
    .elf_name = NULL,
    .elf_stdin = NULL, 
    .elf_stdout = NULL,
    .elf_stderr = NULL,
    .elf_argc = 0,
    .elf_argvs = NULL,
    .sdl_upscale = 1};
// clang-format on

static void print_usage(void) {
    printf("  -u              Specify SDL window upscale\n");
    printf("  -i              Specify ELF stdin\n");
    printf("  -o              Specify ELF stdout\n");
    printf("  -e              Specify ELF stderr\n");
    printf("  -f              Specify ELF file name (if need to specify args place them\
\n                          in \"\" for example \"file -a 1 -b 2\")\n");
}

static void process_f_option(char *arg) {
    int argc = 1;
    va_list argvs;
    if (strchr(arg, ' ')) {
        char *token = strtok(arg, " ");
        while (token != NULL) {
            printf("Token: %s\n", token);
            token = strtok(NULL, " ");
        }
    ctx.elf_argc = argc;
    ctx.elf_argvs = argvs;
    } else {
        // Treat as single string
        ctx.elf_name = arg;
    }
}

void ctx_init(int argc, char *argv[]) {
    int opt;
    int u;
    const char *in;
    while ((opt = getopt(argc, argv, "u:i:o:e:f:h")) != -1) {
        switch (opt) {
        case 'u':
            ctx.sdl_upscale = atoi(optarg);
            printf("%d\n", ctx.sdl_upscale);
            break;
        case 'i':
            ctx.elf_stdin = optarg;
            break;
        case 'o':
            ctx.elf_stdout = optarg;
            break;
        case 'e':
            ctx.elf_stderr = optarg;
            break;
        case 'f':
            process_f_option(optarg);
            break;
        case 'h':
            print_usage();
            exit(EXIT_SUCCESS);
        case '?':
            if (optopt == 'f') {
                print_usage();
                exit(EXIT_FAILURE);
            }
        default:
            print_usage();
            exit(EXIT_FAILURE);
        }
    }

    if (ctx.elf_name == NULL) {
        fprintf(stderr, "  -f is mandatory.\n\n");
        print_usage();
        exit(EXIT_FAILURE);
    }
}
