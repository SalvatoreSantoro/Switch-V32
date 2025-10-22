#include "args.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern Args_Context ctx;


#define CTX_CRASH(str)                                                                                                 \
    do {                                                                                                               \
        fprintf(stderr, "ERROR PARSING PARAMETERS: %s\n", str);                                                        \
        exit(EXIT_FAILURE);                                                                                            \
    } while (0)

// clang-format off

// clang-format on

static void print_usage(void) {
    printf("  -u              Specify SDL window upscale\n");
    printf("  -i              Specify ELF stdin\n");
    printf("  -o              Specify ELF stdout\n");
    printf("  -e              Specify ELF stderr\n");
    printf("  -d              Activate debug\n");
    printf("  -c              Number of cores (IGNORED IN USER MODE)\n");
    printf("  -f              Specify ELF file name (if need to specify args place them\
\n                          in \"\" for example \"file -a 1 -b 2\")\n");
    printf("  -b              If specified, the file must be a binary(IGNORED IN USER MODE)\n");
}

void ctx_init(int argc, char *argv[]) {
    int opt;
    int i = 0;
    unsigned int upscale;
    while ((opt = getopt(argc, argv, "u:i:o:e:f:c:b:dh")) != -1) {
        switch (opt) {
        case 'u':
            upscale = atoi(optarg);
            if (upscale != 0)
                ctx.sdl_upscale = upscale;
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
            ctx.elf_args = optarg;
            while ((ctx.elf_args[i] != '\0') && (ctx.elf_args[i] != ' ') && (i < PATH_MAX)) {
                ctx.elf_name[i] = ctx.elf_args[i];
                i++;
            }
            if (i == PATH_MAX)
                CTX_CRASH("File path name is too big.");
            ctx.elf_name[i] = '\0';
            break;
        case 'c':
#ifdef SUPERVISOR
            ctx.cores = atoi(optarg);
#endif
            break;
        case 'd':
            ctx.debug = true;
            break;
		case 'b':
#ifdef SUPERVISOR
			ctx.binary = true;
#endif
			break;
        case 'h':
            print_usage();
            exit(EXIT_SUCCESS);
        case '?':
        default:
            print_usage();
            exit(EXIT_FAILURE);
        }
    }

    if (ctx.elf_args == NULL)
        CTX_CRASH("-f option is mandatory");
}
