#ifndef _GDB_BUILD_H
#define _GDB_BUILD_H

#include "buffer.h"
#include "data.h"
#include <stdbool.h>

#define NUM_COMMANDS 3

typedef struct Builder Builder;

typedef void *(*Callback_Fun)(void *);
typedef void (*Builder_Fun)(Builder *, PKT_Data *);

typedef enum {
    BUILD_OK,
    BUILD_OOM,
} build_ret;

struct Builder {
    unsigned int command_idx;
    PKT_Buffer *buffer;
    char *commands;
    Builder_Fun builders[NUM_COMMANDS];
    Callback_Fun callbacks[NUM_COMMANDS];
    char *supported;
    char *vcont;
};

void gdb_builder_init(Builder *builder, PKT_Buffer *output_buffer);

void gdb_builder_reset(Builder *build);

void gdb_builder_build_resp(Builder *builder, PKT_Data *pkt_data, bool ack_enabled);

#endif
