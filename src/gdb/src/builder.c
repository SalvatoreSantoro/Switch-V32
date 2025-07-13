#include "builder.h"
#include "buffer.h"
#include "data.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void build_v(Builder *builder, PKT_Data *pkt_data);
static void build_q(Builder *builder, PKT_Data *pkt_data);
static void build_Q(Builder *builder, PKT_Data *pkt_data);

// SAME ORDER AS THE CHARS IN THE "BUILDER->COMMANDS" STRING
// IMPORTANT, CHAR INDEX USED TO DISPATCH CORRECT FUNCTION
// OF THE ARRAY
void gdb_builder_init(Builder *builder, PKT_Buffer *buff) {
    builder->buffer = buff;
    builder->commands = "vqQ";
    builder->builders[0] = build_v;
    builder->builders[1] = build_q;
    builder->builders[2] = build_Q;
    builder->supported =
        "stubfeature;PacketSize=10000;qXfer:features:read+;qXfer:memory-map:read+;QStartNoAckMode+;swbreak+"
        "vContSupported+;multiprocess-;hwbreak-;xmlRegisters+";
    builder->vcont = "vCont;c;s";
}

static int strIdx(const char *str, const char c) {
    int idx = 0;
    while (*(str + idx) != '\0') {
        if (str[idx] == c)
            return idx;
        idx += 1;
    }
    return -1;
}

#define build_unsupported(b) (gdb_buff_append(b, "", 0))

static void build_Q(Builder *builder, PKT_Data *pkt_data) {
    if (strcmp(pkt_data->command + 1, "StartNoAckMode") == 0)
        gdb_buff_append(builder->buffer, "OK", 2);
    else
        build_unsupported(builder->buffer);
}

static void build_v(Builder *builder, PKT_Data *pkt_data) {
    if (strcmp(pkt_data->command + 1, "Cont?") == 0)
        gdb_buff_append(builder->buffer, builder->vcont, strlen(builder->vcont));
    else
        build_unsupported(builder->buffer);
}

static void build_q(Builder *builder, PKT_Data *pkt_data) {
    if (strcmp(pkt_data->command + 1, "Supported") == 0)
        gdb_buff_append(builder->buffer, builder->supported, strlen(builder->supported));
    else
        build_unsupported(builder->buffer);
}

void gdb_builder_build_resp(Builder *builder, PKT_Data *pkt_data, bool ack_enabled) {
    uint8_t checksum;
    char hex_checksum[3];
    int idx;

    if (ack_enabled)
        gdb_buff_append(builder->buffer, "+$", 2);
    else
        gdb_buff_append(builder->buffer, "$", 1);

    builder->buffer->start_pkt_data = builder->buffer->filled;

    // build resp
    idx = strIdx(builder->commands, pkt_data->command[0]);

    if (idx == -1) // unsupported
        build_unsupported(builder->buffer);
    else {
        builder->command_idx = idx;
        // call the correct builder
        builder->builders[idx](builder, pkt_data);
    }

    builder->buffer->end_pkt_data = builder->buffer->filled;

    gdb_buff_append(builder->buffer, "#", 1);

    checksum = gdb_buff_checksum(builder->buffer);
    sprintf(hex_checksum, "%02x", checksum);
    gdb_buff_append(builder->buffer, hex_checksum, 2);
}
