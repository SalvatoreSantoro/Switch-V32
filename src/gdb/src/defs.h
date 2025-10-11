#ifndef DEFS_H
#define DEFS_H
#include <stdlib.h>

// default
#define DEFAULT_BUFF_SIZE 1024
#define DEFAULT_SOCKET_IO_SIZE 1024
#define DEFAULT_PORT 420 
#define DEFAULT_SMP 1
#define DEFAULT_PAR_SIZE 2

// buffers are shrinked when using reset
// and their size is bigger than this parameter
#define MAX_BUFF_DATA_SIZE 4096
#define MAX_DATA_PARAMS_SIZE 10
#define MAX_SMP 16
#define MAX_BREAKPOINTS 128

#define SAD_ASSERT(exp, str)                                                                                           \
    do {                                                                                                               \
        if (!(exp)) {                                                                                                  \
            printf("SAD ASSERT FAILED: %s: %d: %s\n", __FILE__, __LINE__, str);                                        \
            exit(EXIT_FAILURE);                                                                                        \
        }                                                                                                              \
    } while (0)

#endif
