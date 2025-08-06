#ifndef BUILDER_H 
#define BUILDER_H

#include "pkt_buffer.h"
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

void sad_builder_init(Builder *builder, PKT_Buffer *output_buffer);

void sad_builder_deinit(Builder *build);

void sad_builder_build_resp(Builder *builder, PKT_Data *pkt_data, bool ack_enabled);

void sad_builder_reset(Builder* builder);

#endif
