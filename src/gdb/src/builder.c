#include "builder.h"
#include "buffer.h"
#include "data.h"
#include "defs.h"
#include "stub.h"
#include "utils.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GET_CMD_PARAM(p_d) (p_d->command + 1)
#define GET_PARAM_1(i)     (pkt_data->params[i].param1)
#define GET_PARAM_2(i)     (pkt_data->params[i].param2)

// define builder functions
#define X(s, ch) static void build_##s(Builder *builder, PKT_Data *pkt_data);
SUPPORTED_CMDS
#undef X

void sad_builder_init(Builder *builder, PKT_Buffer *output_buffer, Sys_Ops sys_ops, Sys_Conf sys_conf, Set_Ack ack_f) {
    builder->pkt_buffer = output_buffer;
    builder->sys_ops = sys_ops;
    builder->sys_conf = sys_conf;
    builder->selected_core = 0;
    builder->ack_f = ack_f;
    if (sys_conf.arch == RV32) {
        builder->cached_regs_bytes = sys_conf.regs_num * 4;
        builder->cached_regs_str_bytes = (builder->cached_regs_bytes * 2);
    }

#define X(s, ch) builder->supported_builders[COMMAND_##s] = build_##s;
    SUPPORTED_CMDS
#undef X
}

// SAD_EXTEND START "Add new response builder here"
static void build_unsupported(Builder *builder, PKT_Data *pkt_data) {
    sad_buff_append_str(builder->pkt_buffer, "");
}

static void build_g(Builder *builder, PKT_Data *pkt_data) {
    size_t regs_sz = builder->cached_regs_bytes;
    size_t output_sz = builder->cached_regs_str_bytes;
    byte regs[regs_sz];
    char output[output_sz];
    // Read regs
    builder->sys_ops.read_regs(regs, regs_sz, builder->selected_core);
    // Convert to string;
    sad_bytes_to_hex_chars(output, regs, output_sz, regs_sz);
    // Reply
    sad_buff_append(builder->pkt_buffer, output, output_sz);
};

static void build_G(Builder *builder, PKT_Data *pkt_data) {
    // format: G XX...
    const char *regs_str = GET_CMD_PARAM(pkt_data);
    size_t regs_sz = builder->cached_regs_bytes;
    byte regs[regs_sz];
    util_ret ret;

    ret = sad_hex_str_to_bytes(regs, regs_str, regs_sz);

    if (ret != UTIL_OK) {
        sad_buff_append_str(builder->pkt_buffer, "E");
    } else {
        builder->sys_ops.write_regs(regs, regs_sz, builder->selected_core);
        // reply OK
        sad_buff_append_str(builder->pkt_buffer, "OK");
    }
};

static void build_m(Builder *builder, PKT_Data *pkt_data) {
    // format: m addr,length

    const char *addr_str = GET_CMD_PARAM(pkt_data);
    const char *length_str = GET_PARAM_1(0);
    size_t length = strtol(length_str, NULL, 16);
    uint32_t addr = strtol(addr_str, NULL, 16);

    size_t output_sz = length * 2;
    char output[output_sz];

    byte mem[length];

    builder->sys_ops.read_mem(mem, length, addr);

    sad_bytes_to_hex_chars(output, mem, output_sz, length);
    sad_buff_append(builder->pkt_buffer, output, output_sz);
};

static void build_M(Builder *builder, PKT_Data *pkt_data) {
    // formt: M addr,length:XX

    const char *addr_str = GET_CMD_PARAM(pkt_data);
    const char *length_str = GET_PARAM_1(0);
    const char *data_str = GET_PARAM_1(1);

    util_ret ret;

    uint32_t addr = strtol(addr_str, NULL, 16);
    size_t length = strtol(length_str, NULL, 16);

    byte data[length];

    // prepare data to write
    ret = sad_hex_str_to_bytes(data, data_str, length);
    if (ret != UTIL_OK)
        sad_buff_append_str(builder->pkt_buffer, "E");

    builder->sys_ops.write_mem(data, length, addr);
    sad_buff_append_str(builder->pkt_buffer, "OK");
};

static void build_qstmrk(Builder *builder, PKT_Data *pkt_data) {
    sad_buff_append_str(builder->pkt_buffer, "S05");
}

static void build_Q(Builder *builder, PKT_Data *pkt_data) {
    if (strcmp(pkt_data->command, "QStartNoAckMode") == 0) {
        builder->ack_f(false);
        sad_buff_append_str(builder->pkt_buffer, "OK");
    } else
        build_unsupported(builder, pkt_data);
}

static void build_q(Builder *builder, PKT_Data *pkt_data) {
    if (strcmp(pkt_data->command, "qSupported") == 0) {
        sad_buff_append_str(builder->pkt_buffer, "swbreak+;vCont+;QStartNoAckMode+");
    }
    if (strcmp(pkt_data->command, "qfThreadInfo") == 0) {
        char core_id_str[4];
        int i;
        sad_buff_append_str(builder->pkt_buffer, "m");
        for (i = 0; i < builder->sys_conf.smp - 1; i++) {
            sprintf(core_id_str, "%d,", i);
            sad_buff_append_str(builder->pkt_buffer, core_id_str);
        }
        sprintf(core_id_str, "%d", i);
        sad_buff_append_str(builder->pkt_buffer, core_id_str);
    }
    if (strcmp(pkt_data->command, "qsThreadInfo") == 0) {
        sad_buff_append_str(builder->pkt_buffer, "l");
    }

    if (strcmp(pkt_data->command, "qThreadExtraInfo") == 0) {
        const char *thread_id_str = GET_PARAM_1(0);
        sad_buff_append_str(builder->pkt_buffer, thread_id_str);
    }

    if (strcmp(pkt_data->command, "qC") == 0) {
        char response[10];
        sprintf(response, "QC%d", builder->selected_core);
        sad_buff_append_str(builder->pkt_buffer, response);
    }
    if (strcmp(pkt_data->command, "qSymbol") == 0) {
        sad_buff_append_str(builder->pkt_buffer, "OK");
    } else
        build_unsupported(builder, pkt_data);
}

static void build_v(Builder *builder, PKT_Data *pkt_data) {
    if (strcmp(pkt_data->command, "vCont?") == 0) {
        sad_buff_append_str(builder->pkt_buffer, "s;c");

    } else if (strcmp(pkt_data->command, "vCont") == 0) {
        int fill = pkt_data->params_filled;

        if (fill >= 1) {
            const char *thread_id_str = GET_PARAM_1(0);
            int thread_id = strtol(thread_id_str, NULL, 16);
            switch (GET_PARAM_1(0)[0]) {
            case 'c':
                // this is the case of continue all so or just "c" or "c:-1"
                if ((fill == 1) || ((fill >= 2) && (thread_id == -1)))
                    builder->sys_ops.cores_continue();
                else
                    builder->sys_ops.core_continue(thread_id);
                break;
            case 's':
                if (thread_id == -1) {
                    sad_buff_append_str(builder->pkt_buffer, "E");
                    return;
                }
                builder->sys_ops.core_step(thread_id);
                break;
            default:
                build_unsupported(builder, pkt_data);
                break;
            }

        } else {
            sad_buff_append_str(builder->pkt_buffer, "E");
            return;
        }

        sad_buff_append_str(builder->pkt_buffer, "S05");
        //
    } else
        build_unsupported(builder, pkt_data);
}

static void build_T(Builder *builder, PKT_Data *pkt_data) {
    sad_buff_append_str(builder->pkt_buffer, "OK");
}

static void build_H(Builder *builder, PKT_Data *pkt_data) {

    if (strncmp(pkt_data->command, "Hg", 2) == 0) {
        const char *thread_id_str = pkt_data->command + 2;
        int thread_id = strtol(thread_id_str, NULL, 10);

        if (thread_id != -1)
            builder->selected_core = thread_id;

        sad_buff_append_str(builder->pkt_buffer, "OK");
    } else {
        build_unsupported(builder, pkt_data);
    }
}

static void build_Z(Builder *builder, PKT_Data *pkt_data) {
    // format: Z type,addr,kind
    /**/
    /* const char *str_type = GET_CMD_PARAM(pkt_data); */
    /* const char *str_addr = pkt_data->params[0].param1; */
    /* const char *str_kind = pkt_data->params[1].param1; */
    /**/
    /* int type = atoi(str_type); */
    /* int kind = atoi(str_kind); */
    /* long addr = strtol(str_addr, NULL, 16); */
    /**/
    /* if ((type < 0) || (type > 4) || (kind != 4)) { */
    /*     // compressed (kind == 2) is unsupported atm */
    /*     sad_buff_append_str(builder->pkt_buffer, "E"); */
    /*     return; */
    /* } */
    /**/
    /* // read current content */
    /* READ_MEM_CBK_t *r_handler = GET_HANDL(builder->cbks, READ_MEM_CBK); */
    /* r_handler->addr = addr; */
    /* r_handler->length = kind; */
    /**/
    /* sad_callbacks_dispatch(builder->cbks, READ_MEM_CBK); */
    /**/
    /* size_t filled; */
    /* unsigned char prova[kind]; */
    /* sad_buff_read_prep(r_handler->output, &filled); */
    /**/
    /* printf("PROVA BREAK: %X\n", *prova); */
    /**/
    /* // write breakpoint instruction */
    /* WRITE_MEM_CBK_t *w_handler = GET_HANDL(builder->cbks, WRITE_MEM_CBK); */
    /* // ebreak encoding */
    /* uint32_t tmp_data = 0x00100073; */
    /* w_handler->addr = addr; */
    /* w_handler->length = kind; */
    /* w_handler->data = (unsigned char *) &tmp_data; */
    /**/
    /* sad_callbacks_dispatch(builder->cbks, WRITE_MEM_CBK); */
    /* sad_pkt_buff_append_str(builder->pkt_buffer, "OK"); */
}

// SAD_EXTEND END

void sad_builder_build_resp(Builder *builder, PKT_Data *pkt_data, bool ack_enabled) {
    uint8_t checksum;
    char hex_checksum[3];
    cmd_type cmd;

    if (ack_enabled)
        sad_buff_append_str(builder->pkt_buffer, "+$");
    else
        sad_buff_append_str(builder->pkt_buffer, "$");

    builder->pkt_buffer->start_pkt_data = builder->pkt_buffer->filled;

    // build resp
    cmd = supported_idx(pkt_data->command[0]);

    // call the correct builder
    builder->supported_builders[cmd](builder, pkt_data);

    builder->pkt_buffer->end_pkt_data = builder->pkt_buffer->filled;

    sad_buff_append_str(builder->pkt_buffer, "#");

    checksum = sad_buff_checksum(builder->pkt_buffer);
    sprintf(hex_checksum, "%02x", checksum);
    sad_buff_append_str(builder->pkt_buffer, hex_checksum);
}
