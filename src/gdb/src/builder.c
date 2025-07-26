#include "builder.h"
#include "buffer.h"
#include "data.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void build_v(Builder *builder, PKT_Data *pkt_data);
static void build_q(Builder *builder, PKT_Data *pkt_data);
static void build_Q(Builder *builder, PKT_Data *pkt_data);
static void build_H(Builder *builder, PKT_Data *pkt_data);
static void build_qst_mrk(Builder *builder, PKT_Data *pkt_data);
static void build_g(Builder *builder, PKT_Data *pkt_data);
static void build_p(Builder *builder, PKT_Data *pkt_data);

REGS_CBK_t gas = {0};

// SAME ORDER AS THE CHARS IN THE "BUILDER->COMMANDS" STRING
// IMPORTANT, CHAR INDEX USED TO DISPATCH CORRECT FUNCTION
// OF THE ARRAY
void gdb_builder_init(Builder *builder, PKT_Buffer *buff) {
    builder->buffer = buff;
    builder->commands = "qQH?gpv";
    builder->builders[0] = build_q;
    builder->builders[1] = build_Q;
    builder->builders[2] = build_H;
    builder->builders[3] = build_qst_mrk;
    builder->builders[4] = build_g;
    builder->builders[5] = build_p;
    builder->builders[6] = build_v;
    builder->supported = "PacketSize=3000;QStartNoAckMode+;swbreak+;";
    builder->vcont = "vCont;c;s";
}

void gdb_builder_register_callbacks(Builder *builder) {
    for (const Callback_Registration *reg = &__start_sad_callbacks; reg < &__stop_sad_callbacks; ++reg) {
        builder->callbacks[reg->type].fun = reg->cbk.fun;
        builder->callbacks[reg->type].arg = reg->cbk.arg;
    }
}



#define build_unsupported(b) (gdb_buff_append_str(b, ""))

static void build_p(Builder *builder, PKT_Data *pkt_data) {
    // unimplemented
}

static void build_g(Builder *builder, PKT_Data *pkt_data) {
    // unimplemented
};

static void build_qst_mrk(Builder *builder, PKT_Data *pkt_data) {
    gdb_buff_append_str(builder->buffer, "S05");
}

static void build_H(Builder *builder, PKT_Data *pkt_data) {
    // unsupported for now
    build_unsupported(builder->buffer);
}

static void build_Q(Builder *builder, PKT_Data *pkt_data) {
    Callback *ctx;

    if (strcmp(pkt_data->command + 1, "StartNoAckMode") == 0) {
        // unimplemented
    } else
        build_unsupported(builder->buffer);
}

static void build_v(Builder *builder, PKT_Data *pkt_data) {
    // unsupported for now
    build_unsupported(builder->buffer);
}

static void build_q(Builder *builder, PKT_Data *pkt_data) {
    builder->callbacks[REGS_CBK].fun(builder->callbacks[REGS_CBK].arg, (void *) &gas);

    if (strcmp(pkt_data->command + 1, "Supported") == 0)
        gdb_buff_append_str(builder->buffer, builder->supported);
    else
        build_unsupported(builder->buffer);
}

void gdb_builder_build_resp(Builder *builder, PKT_Data *pkt_data, bool ack_enabled) {
    uint8_t checksum;
    char hex_checksum[3];
    int idx;

    if (ack_enabled)
        gdb_buff_append_str(builder->buffer, "+$");
    else
        gdb_buff_append_str(builder->buffer, "$");

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

    gdb_buff_append_str(builder->buffer, "#");

    checksum = gdb_buff_checksum(builder->buffer);
    sprintf(hex_checksum, "%02x", checksum);
    gdb_buff_append_str(builder->buffer, hex_checksum);
}
