#include "builder.h"
#include "buffer.h"
#include "callback.h"
#include "data.h"
#include "defs.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GET_HANDL(c, t)    (c[t].handler_data)
#define GET_CMD_PARAM(p_d) (p_d->command + 1)

int gdb_util_str_to_hex(unsigned char *bytes, const char *str, size_t bytes_size) {
    char c;
    size_t str_len = strlen(str);
    // if the assumptions of the data structures made
    // in order to accomodate the strings sent by GDB
    // are wrong, there is something broken in the implementation
    // (+1 to ceil)

    // could use errors
    SAD_ASSERT(bytes_size >= ((str_len + 1) / 2), "GDB sent ");

    // 0 first hex symbol
    // 1 second hex symbol
    bool state = (str_len % 2) == 0 ? 1 : 0;
    unsigned char tmp_val = 0;
    size_t bytes_idx = 0;
    size_t i = 0;

    while (*(str + i) != '\0') {
        c = *(str + i);
        if (c >= '0' && c <= '9')
            c -= '0';
        else if (c >= 'A' && c <= 'F')
            c = c - 'A' + 10;
        else if (c >= 'a' && c <= 'f')
            c = c - 'a' + 10;
        else
            return -1;

        if (state) {
            c <<= 4;
            tmp_val = c;
        } else {
            tmp_val += c;
            *(bytes + bytes_idx) = tmp_val;
            bytes_idx += 1;
        }

        state = !state;
        i += 1;
    }
    // reset the remaining area
    while (bytes_idx != bytes_size) {
        *(bytes + bytes_idx) = 0;
        bytes_idx += 1;
    }

    return 0;
}

// define builder functions
#define X(s, ch) static void build_##s(Builder *builder, PKT_Data *pkt_data);
SUPPORTED_CMDS
#undef X

void gdb_builder_init(Builder *builder, PKT_Buffer *pkt_buff) {
    builder->pkt_buffer = pkt_buff;
#define X(s, ch) builder->supported_builders[COMMAND_##s] = build_##s;
    SUPPORTED_CMDS
#undef X

    gdb_callbacks_init(builder->cbks, builder->pkt_buffer);
}

void gdb_builder_deinit(Builder *builder) {
    gdb_callbacks_deinit(builder->cbks);
}

// SAD_EXTEND START "Add new response builder here"
static void build_unsupported(Builder *builder, PKT_Data *pkt_data) {
    gdb_buff_append_str(builder->pkt_buffer, "");
}

static void build_g(Builder *builder, PKT_Data *pkt_data) {
    gdb_callbacks_dispatch(builder->cbks, READ_REGS_CBK);
};

static void build_G(Builder *builder, PKT_Data *pkt_data) {
    // format: G XX...
    const char *registers = GET_CMD_PARAM(pkt_data);
    const char *pc = registers + REGS_STR_SIZE;

    // get request handler
    WRITE_REGS_CBK_t *handler = GET_HANDL(builder->cbks, WRITE_REGS_CBK);

    // pass the data from parsed request to handler
    gdb_util_str_to_hex((unsigned char *) handler->regs, registers, sizeof(handler->regs));

    gdb_callbacks_dispatch(builder->cbks, WRITE_REGS_CBK);
    // reply OK
    gdb_buff_append_str(builder->pkt_buffer, "OK");
};

static void build_m(Builder *builder, PKT_Data *pkt_data) {
    // format: m addr,length

    const char *addr = GET_CMD_PARAM(pkt_data);
    const char *length = pkt_data->params[0].param1;

    READ_MEM_CBK_t *handler = GET_HANDL(builder->cbks, READ_MEM_CBK);

    handler->addr = strtol(addr, NULL, 16);
    handler->length = strtol(length, NULL, 16);

    gdb_callbacks_dispatch(builder->cbks, READ_MEM_CBK);
};

static void build_M(Builder *builder, PKT_Data *pkt_data) {
    // formt: M addr,length:XX

    const char *addr = GET_CMD_PARAM(pkt_data);
    const char *length = pkt_data->params[0].param1;
    const char *str_data = pkt_data->params[1].param1;

    WRITE_MEM_CBK_t *handler = GET_HANDL(builder->cbks, WRITE_MEM_CBK);

    handler->addr = strtol(addr, NULL, 16);
    handler->length = strtol(length, NULL, 16);

    unsigned char tmp_data[handler->length];

    gdb_util_str_to_hex(tmp_data, str_data, handler->length);
    handler->data = tmp_data;

    gdb_callbacks_dispatch(builder->cbks, WRITE_MEM_CBK);
    gdb_buff_append_str(builder->pkt_buffer, "OK");
};

static void build_qstmrk(Builder *builder, PKT_Data *pkt_data) {
    gdb_buff_append_str(builder->pkt_buffer, "S05");
}

static void build_Q(Builder *builder, PKT_Data *pkt_data) {
    if (strcmp(pkt_data->command, "QStartNoAckMode") == 0) {
        // unimplemented
    } else
        build_unsupported(builder, pkt_data);
}

static void build_q(Builder *builder, PKT_Data *pkt_data) {
    if (strcmp(pkt_data->command, "qSupported") == 0) {
        gdb_buff_append_str(builder->pkt_buffer, "swbreak+;vCont+");
    }
    if (strcmp(pkt_data->command, "qfThreadInfo") == 0) {
        gdb_buff_append_str(builder->pkt_buffer, "0l");
    }
    if (strcmp(pkt_data->command, "qC") == 0) {
        gdb_buff_append_str(builder->pkt_buffer, "0");
    }
    if (strcmp(pkt_data->command, "qSymbol") == 0) {
        gdb_buff_append_str(builder->pkt_buffer, "OK");
    } else

        build_unsupported(builder, pkt_data);
}

static void build_v(Builder *builder, PKT_Data *pkt_data) {
    if (strcmp(pkt_data->command, "vCont?") == 0) {
        gdb_buff_append_str(builder->pkt_buffer, "s");
    } else if (strcmp(pkt_data->command, "vCont") == 0) {
        gdb_buff_append_str(builder->pkt_buffer, "E01");
    } else
        build_unsupported(builder, pkt_data);
}

// SAD_EXTEND END

void gdb_builder_build_resp(Builder *builder, PKT_Data *pkt_data, bool ack_enabled) {
    uint8_t checksum;
    char hex_checksum[3];
    cmd_type cmd;

    if (ack_enabled)
        gdb_buff_append_str(builder->pkt_buffer, "+$");
    else
        gdb_buff_append_str(builder->pkt_buffer, "$");

    builder->pkt_buffer->start_pkt_data = builder->pkt_buffer->filled;

    // build resp
    cmd = supported_idx(pkt_data->command[0]);

    // call the correct builder
    builder->supported_builders[cmd](builder, pkt_data);

    builder->pkt_buffer->end_pkt_data = builder->pkt_buffer->filled;

    gdb_buff_append_str(builder->pkt_buffer, "#");

    checksum = gdb_buff_checksum(builder->pkt_buffer);
    sprintf(hex_checksum, "%02x", checksum);
    gdb_buff_append_str(builder->pkt_buffer, hex_checksum);
}
