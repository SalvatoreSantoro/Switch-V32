#ifndef DEFS_H
#define DEFS_H
#include <stdlib.h>

// types
#define byte unsigned char

#define DEFAULT_PAR_SIZE 2

// buffers are shrinked when using reset
// and their size is bigger than this parameter
#define MAX_BUFF_DATA_SIZE 4096

#define NUM_REGS      33
//#define REGS_STR_SIZE (NUM_REGS * 8)
#define PC_STR_SIZE   (8)

#define SAD_ASSERT(exp, str)                                                                                           \
    do {                                                                                                               \
        if (!(exp)) {                                                                                                  \
            printf("SAD ASSERT FAILED: %s: %d: %s\n", __FILE__, __LINE__, str);                                        \
            exit(EXIT_FAILURE);                                                                                        \
        }                                                                                                              \
    } while (0)

#endif
