#include "builder.h"
#include "buffer.h"
#include "callback.h"
#include "data.h"
#include "defs.h"
#include "pkt_buffer.h"
#include "utils.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GET_HANDL(c, t)    (c[t].handler_data)
#define GET_CMD_PARAM(p_d) (p_d->command + 1)

// define builder functions
#define X(s, ch) static void build_##s(Builder *builder, PKT_Data *pkt_data);
SUPPORTED_CMDS
#undef X

static void build_from_handler(PKT_Buffer *pkt_buffer, Buffer *buffer) {
    util_ret ret;
    char result_chars[2 * buffer->filled];
    ret = sad_bytes_to_hex_chars(result_chars, buffer->data, 2 * buffer->filled, buffer->filled);
    if (ret != UTIL_OK)
        sad_pkt_buff_append_str(pkt_buffer, "E");
    else
        sad_pkt_buff_append(pkt_buffer, (byte *) result_chars, 2 * buffer->filled);
}

void sad_builder_init(Builder *builder, PKT_Buffer *pkt_buff) {
    builder->pkt_buffer = pkt_buff;
#define X(s, ch) builder->supported_builders[COMMAND_##s] = build_##s;
    SUPPORTED_CMDS
#undef X

    sad_callbacks_init(builder->cbks);
}

void sad_builder_deinit(Builder *builder) {
    sad_callbacks_deinit(builder->cbks);
}

// SAD_EXTEND START "Add new response builder here"
static void build_unsupported(Builder *builder, PKT_Data *pkt_data) {
    sad_pkt_buff_append_str(builder->pkt_buffer, "");
}

static void build_g(Builder *builder, PKT_Data *pkt_data) {
    READ_REGS_CBK_t *handler = GET_HANDL(builder->cbks, READ_REGS_CBK);
    sad_callbacks_dispatch(builder->cbks, READ_REGS_CBK);
    build_from_handler(builder->pkt_buffer, handler->output);
};

static void build_G(Builder *builder, PKT_Data *pkt_data) {
    // format: G XX...
    const char *registers = GET_CMD_PARAM(pkt_data);
    util_ret ret;

    // get request handler
    WRITE_REGS_CBK_t *handler = GET_HANDL(builder->cbks, WRITE_REGS_CBK);

    // pass the data from parsed request to handler
    ret = sad_hex_str_to_bytes((byte *) handler->regs, registers, sizeof(handler->regs));

    if (ret != UTIL_OK) {
        sad_pkt_buff_append_str(builder->pkt_buffer, "E");
    } else {
        sad_callbacks_dispatch(builder->cbks, WRITE_REGS_CBK);
        // reply OK
        sad_pkt_buff_append_str(builder->pkt_buffer, "OK");
    }
};

static void build_m(Builder *builder, PKT_Data *pkt_data) {
    // format: m addr,length

    const char *addr = GET_CMD_PARAM(pkt_data);
    const char *length = pkt_data->params[0].param1;

    READ_MEM_CBK_t *handler = GET_HANDL(builder->cbks, READ_MEM_CBK);

    handler->addr = strtol(addr, NULL, 16);
    handler->length = strtol(length, NULL, 16);

    sad_callbacks_dispatch(builder->cbks, READ_MEM_CBK);

    build_from_handler(builder->pkt_buffer, handler->output);
};

static void build_M(Builder *builder, PKT_Data *pkt_data) {
    // formt: M addr,length:XX

    const char *addr = GET_CMD_PARAM(pkt_data);
    const char *length = pkt_data->params[0].param1;
    const char *str_data = pkt_data->params[1].param1;
    util_ret ret;

    WRITE_MEM_CBK_t *handler = GET_HANDL(builder->cbks, WRITE_MEM_CBK);

    handler->addr = strtol(addr, NULL, 16);
    handler->length = strtol(length, NULL, 16);

    // prepare data to write
    byte tmp_data[handler->length];
    ret = sad_hex_str_to_bytes(tmp_data, str_data, handler->length);
    if (ret != UTIL_OK)
        sad_pkt_buff_append_str(builder->pkt_buffer, "E");

    handler->data = tmp_data;

    sad_callbacks_dispatch(builder->cbks, WRITE_MEM_CBK);

    sad_pkt_buff_append_str(builder->pkt_buffer, "OK");
};

static void build_qstmrk(Builder *builder, PKT_Data *pkt_data) {
    sad_pkt_buff_append_str(builder->pkt_buffer, "S05");
}

static void build_Q(Builder *builder, PKT_Data *pkt_data) {
    if (strcmp(pkt_data->command, "QStartNoAckMode") == 0) {
        // unimplemented
    } else
        build_unsupported(builder, pkt_data);
}

static void build_q(Builder *builder, PKT_Data *pkt_data) {
    if (strcmp(pkt_data->command, "qSupported") == 0) {
        sad_pkt_buff_append_str(builder->pkt_buffer, "swbreak+;vCont+");
    }
    if (strcmp(pkt_data->command, "qfThreadInfo") == 0) {
        sad_pkt_buff_append_str(builder->pkt_buffer, "m0,1,2");
    }
    if (strcmp(pkt_data->command, "qsThreadInfo") == 0) {
        sad_pkt_buff_append_str(builder->pkt_buffer, "l");
    }
    if (strcmp(pkt_data->command, "qC") == 0) {
        sad_pkt_buff_append_str(builder->pkt_buffer, "0");
    }
    if (strcmp(pkt_data->command, "qSymbol") == 0) {
        sad_pkt_buff_append_str(builder->pkt_buffer, "OK");
    } else
        build_unsupported(builder, pkt_data);
}

static void build_v(Builder *builder, PKT_Data *pkt_data) {
    if (strcmp(pkt_data->command, "vCont?") == 0) {
        sad_pkt_buff_append_str(builder->pkt_buffer, "s");
        //build_unsupported(builder, pkt_data);
    } else if (strcmp(pkt_data->command, "vCont") == 0) {
        // 
    } else
        build_unsupported(builder, pkt_data);
}

static void build_T(Builder* builder, PKT_Data* pkt_data){
    sad_pkt_buff_append_str(builder->pkt_buffer, "OK");
}


static void build_Z(Builder *builder, PKT_Data *pkt_data) {
    // format: Z type,addr,kind

    const char *str_type = GET_CMD_PARAM(pkt_data);
    const char *str_addr = pkt_data->params[0].param1;
    const char *str_kind = pkt_data->params[1].param1;

    int type = atoi(str_type);
    int kind = atoi(str_kind);
    long addr = strtol(str_addr, NULL, 16);

    if ((type < 0) || (type > 4) || (kind != 4)) {
        // compressed (kind == 2) is unsupported atm
        sad_pkt_buff_append_str(builder->pkt_buffer, "E");
        return;
    }

    // read current content
    READ_MEM_CBK_t *r_handler = GET_HANDL(builder->cbks, READ_MEM_CBK);
    r_handler->addr = addr;
    r_handler->length = kind;

    sad_callbacks_dispatch(builder->cbks, READ_MEM_CBK);

    size_t filled;
    unsigned char prova[kind];
    sad_buff_read_prep(r_handler->output, &filled);

    printf("PROVA BREAK: %X\n", *prova);

    // write breakpoint instruction
    WRITE_MEM_CBK_t *w_handler = GET_HANDL(builder->cbks, WRITE_MEM_CBK);
    // ebreak encoding
    uint32_t tmp_data = 0x00100073;
    w_handler->addr = addr;
    w_handler->length = kind;
    w_handler->data = (unsigned char *) &tmp_data;

    sad_callbacks_dispatch(builder->cbks, WRITE_MEM_CBK);
    sad_pkt_buff_append_str(builder->pkt_buffer, "OK");
}

// SAD_EXTEND END

void sad_builder_build_resp(Builder *builder, PKT_Data *pkt_data, bool ack_enabled) {
    uint8_t checksum;
    char hex_checksum[3];
    cmd_type cmd;

    if (ack_enabled)
        sad_pkt_buff_append_str(builder->pkt_buffer, "+$");
    else
        sad_pkt_buff_append_str(builder->pkt_buffer, "$");

    builder->pkt_buffer->start_pkt_data = builder->pkt_buffer->buff.filled;

    // build resp
    cmd = supported_idx(pkt_data->command[0]);

    // call the correct builder
    builder->supported_builders[cmd](builder, pkt_data);

    builder->pkt_buffer->end_pkt_data = builder->pkt_buffer->buff.filled;

    sad_pkt_buff_append_str(builder->pkt_buffer, "#");

    checksum = sad_pkt_buff_checksum(builder->pkt_buffer);
    sprintf(hex_checksum, "%02x", checksum);
    sad_pkt_buff_append_str(builder->pkt_buffer, hex_checksum);
}

void sad_builder_reset(Builder *builder) {
    sad_callbacks_reset(builder->cbks);
}
