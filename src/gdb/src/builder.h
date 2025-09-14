#ifndef BUILDER_H
#define BUILDER_H

#include "buffer.h"
#include "stub.h"
#include "data.h"
#include "supported.h"
#include <stdbool.h>

typedef struct Builder Builder;

typedef void (*Builder_Fun)(Builder *, PKT_Data *);

struct Builder {
    PKT_Buffer *pkt_buffer;
    Builder_Fun supported_builders[COMMANDS_COUNT];
    Sys_Ops sys_ops;
    Sys_Conf sys_conf;
    size_t cached_regs_bytes;
    size_t cached_regs_str_bytes;
    int selected_core;
};

void sad_builder_init(Builder *builder, PKT_Buffer *output_buffer, Sys_Ops sys_ops, Sys_Conf sys_conf);

void sad_builder_build_resp(Builder *builder, PKT_Data *pkt_data, bool ack_enabled);

#endif
