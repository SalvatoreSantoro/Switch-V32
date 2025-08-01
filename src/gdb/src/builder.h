#ifndef _GDB_BUILD_H
#define _GDB_BUILD_H

#include "buffer.h"
#include "callback.h"
#include "data.h"
#include "supported.h"
#include <stdbool.h>


typedef struct Builder Builder;

typedef void (*Builder_Fun)(Builder *, PKT_Data *);

struct Builder {
    PKT_Buffer *pkt_buffer;
    Builder_Fun supported_builders[COMMANDS_COUNT];
    Callback cbks[NUM_CBKS];
};

void gdb_builder_init(Builder *builder, PKT_Buffer *output_buffer);

void gdb_builder_deinit(Builder *build);

void gdb_builder_build_resp(Builder *builder, PKT_Data *pkt_data, bool ack_enabled);


#endif
