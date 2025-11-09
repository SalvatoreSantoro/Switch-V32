#include "args.h"
#include "defs.h"
#include "macros.h"
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void print_usage(void) {
    printf("  -u              Specify SDL window upscale\n");
    printf("  -i              Specify ELF stdin\n");
    printf("  -o              Specify ELF stdout\n");
    printf("  -e              Specify ELF stderr\n");
    printf("  -d              Activate debug\n");
    printf("  -c              Number of cores (max %d)\n", MAX_NPROCS);
    printf("  -m			  Memory size in MBs (max %d MB)\n", MAX_MEM_SIZE_MB);
    printf("  -f              Specify ELF file name (if need to specify args place them\
\n                          in \"\" for example \"file -a 1 -b 2\")\n");
    printf("  -b              If specified, the file must be a binary(IGNORED IN USER MODE)\n");
}

void ctx_init(int argc, char *argv[]) {
    int opt;
    int i = 0;
    unsigned int upscale;
    uint64_t mem_tmp;

    while ((opt = getopt(argc, argv, "u:i:o:e:f:c:m:b:dh")) != -1) {
        switch (opt) {
        case 'u':
            upscale = (unsigned int) atoi(optarg);
            if ((upscale == 0) || (upscale > MAX_UPSCALE))
                printf("Upscale value ignored (correct value is between 1 and %d\n", MAX_UPSCALE);
            else
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
                SV32_CRASH("File path name is too big.");
            ctx.elf_name[i] = '\0';
            break;
        case 'c':
#ifdef SUPERVISOR
            ctx.cores = (unsigned int) atoi(optarg);
            if ((ctx.cores == 0) || (ctx.cores > MAX_NPROCS))
                SV32_CRASH("Invalid number of cores.");
#endif
            break;
        case 'm':
            mem_tmp = (uint64_t) atoi(optarg);
            if ((mem_tmp == 0) || (mem_tmp > MAX_MEM_SIZE_MB))
                SV32_CRASH("Invalid memory size.");
            ctx.memory_size = mem_tmp * 1024 * 1024; // compute bytes
            ctx.stack_base = (uint32_t) (ctx.memory_size - PAGE_SIZE);
#ifdef USER
            ctx.brk_limit = (uint32_t) (ctx.memory_size >> 4);
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
        SV32_CRASH("-f option is mandatory");
}
