#include "../stubb_a_dub.h"
#include "sad_gdb_internal.h"
#include "supported.h"
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern SAD_Stub server;

#define GET_CMD_PARAM  (server.pkt_data.command + 1)
#define GET_PARAM_1(i) (server.pkt_data.params[i].param1)
#define GET_PARAM_2(i) (server.pkt_data.params[i].param2)

// define builder functions
#define X(s, ch) static void build_##s(void);
SUPPORTED_CMDS
#undef X

#define LONG_CHECK(val)                                                                                                \
    do {                                                                                                               \
        if (val == LONG_MAX || val == LONG_MIN) {                                                                      \
            sad_buff_append_str(server.output_buffer, "E");                                                            \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

#define LONG_TO_SIZE_CHECK(val)                                                                                        \
    do {                                                                                                               \
        if (val == LONG_MAX || val <= 0) {                                                                             \
            sad_buff_append_str(server.output_buffer, "E");                                                            \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

#define LONG_TO_UINT32_CHECK(val)                                                                                      \
    do {                                                                                                               \
        if (val > UINT32_MAX) {                                                                                        \
            sad_buff_append_str(server.output_buffer, "E");                                                            \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

#define LONG_TO_INT_CHECK(val)                                                                                         \
    do {                                                                                                               \
        if (val > INT_MAX || val < INT_MIN) {                                                                          \
            sad_buff_append_str(server.output_buffer, "E");                                                            \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

#define LONG_TO_UINT(val)                                                                                              \
    do {                                                                                                               \
        if (val > UINT_MAX || val <= 0) {                                                                              \
            sad_buff_append_str(server.output_buffer, "E");                                                            \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

static int fd_set_blocking(bool blocking) {
    int flags = fcntl(server.sad_socket, F_GETFL, 0);

    if (flags == -1)
        return -1;

    if (blocking)
        flags &= ~O_NONBLOCK; // clear nonblocking flag
    else
        flags |= O_NONBLOCK; // set nonblocking flag

    return fcntl(server.sad_socket, F_SETFL, flags);
}

static bool wait_for_halt(unsigned int core_idx) {
    // set the socket as non blocking
    bool breakp_stop = false;
    size_t filled;
    unsigned char *data;
    fd_set_blocking(false);

    while (!breakp_stop) {
        // check GDB stop message
        sad_buff_from_socket(server.input_buffer, server.sad_socket);

        // ^C from gdb
        data = sad_buff_read_prep(server.input_buffer, &filled);

        if (data[filled - 1] == '\x03') {
            server.sys_ops.cores_halt();
            break;
        }

        // check if core halted
        // just check the core_idx because as a convention a breakpoint stops all the threads
        breakp_stop = server.sys_ops.is_halted(core_idx);
    }

    // reset to blocking socket
    fd_set_blocking(true);
    return breakp_stop;
}

void sad_builder_init(void) {
    server.builder.selected_core = 0;

    if (server.sys_conf.arch == RV32) {
        server.builder.cached_regs_bytes = server.sys_conf.regs_num * 4;
        server.builder.cached_regs_str_bytes = (server.builder.cached_regs_bytes * 2);
    }

#define X(s, ch) server.builder.supported_builders[COMMAND_##s] = build_##s;
    SUPPORTED_CMDS
#undef X
}

// SAD_EXTEND START "Add new response builder here"
static void build_unsupported(void) {
    sad_buff_append_str(server.output_buffer, "");
}

static void build_g(void) {
    size_t regs_sz = server.builder.cached_regs_bytes;
    size_t output_sz = server.builder.cached_regs_str_bytes;
    byte regs[regs_sz];
    char output[output_sz];
    // Read regs
    server.sys_ops.read_regs(regs, regs_sz, server.builder.selected_core);
    // Convert to string;
    sad_bytes_to_hex_chars(output, regs, output_sz, regs_sz);
    // Reply
    sad_buff_append(server.output_buffer, output, output_sz);
}

static void build_G(void) {
    // format: G XX...
    const char *regs_str = GET_CMD_PARAM;
    size_t regs_sz = server.builder.cached_regs_bytes;
    byte regs[regs_sz];
    util_ret ret;

    ret = sad_hex_str_to_bytes(regs, regs_str, regs_sz);

    if (ret != UTIL_OK) {
        sad_buff_append_str(server.output_buffer, "E");
    } else {
        server.sys_ops.write_regs(regs, regs_sz, server.builder.selected_core);
        // reply OK
        sad_buff_append_str(server.output_buffer, "OK");
    }
}

static void build_m(void) {
    // format: m addr,length

    const char *addr_str = GET_CMD_PARAM;
    const char *length_str = GET_PARAM_1(0);

    long length = strtol(length_str, NULL, 16);
    long addr = strtol(addr_str, NULL, 16);

    LONG_TO_SIZE_CHECK(length);
    LONG_TO_UINT32_CHECK(addr);

    size_t output_sz = (size_t) length * 2;
    char output[output_sz];

    byte mem[length];

    server.sys_ops.read_mem(mem, (size_t) length, (uint32_t) addr);

    sad_bytes_to_hex_chars(output, mem, output_sz, (size_t) length);
    sad_buff_append(server.output_buffer, output, output_sz);
}

static void build_M(void) {
    // formt: M addr,length:XX

    const char *addr_str = GET_CMD_PARAM;
    const char *length_str = GET_PARAM_1(0);
    const char *data_str = GET_PARAM_1(1);

    util_ret ret;

    long addr = strtol(addr_str, NULL, 16);
    long length = strtol(length_str, NULL, 16);

    LONG_TO_SIZE_CHECK(length);
    LONG_TO_UINT32_CHECK(addr);

    byte data[length];

    // prepare data to write
    ret = sad_hex_str_to_bytes(data, data_str, (size_t) length);
    if (ret != UTIL_OK)
        sad_buff_append_str(server.output_buffer, "E");

    server.sys_ops.write_mem(data, (size_t) length, (uint32_t) addr);
    sad_buff_append_str(server.output_buffer, "OK");
}

static void build_qstmrk(void) {
    sad_buff_append_str(server.output_buffer, "S05");
}

static void build_Q(void) {
    if (strcmp(server.pkt_data.command, "QStartNoAckMode") == 0) {
        server.ack_enabled = false;
        sad_buff_append_str(server.output_buffer, "OK");
    } else
        build_unsupported();
}

static void build_q(void) {
    if (strcmp(server.pkt_data.command, "qSupported") == 0) {
        sad_buff_append_str(server.output_buffer, "swbreak+;vCont+;QStartNoAckMode+");
    }
    if (strcmp(server.pkt_data.command, "qfThreadInfo") == 0) {
        char core_id_str[16];
        unsigned int i;
        sad_buff_append_str(server.output_buffer, "m");
        for (i = 0; i < server.sys_conf.smp - 1; i++) {
            sprintf(core_id_str, "%d,", i);
            sad_buff_append_str(server.output_buffer, core_id_str);
        }
        sprintf(core_id_str, "%d", i);
        sad_buff_append_str(server.output_buffer, core_id_str);
    }
    if (strcmp(server.pkt_data.command, "qsThreadInfo") == 0) {
        sad_buff_append_str(server.output_buffer, "l");
    }

    if (strcmp(server.pkt_data.command, "qThreadExtraInfo") == 0) {
        const char *thread_id_str = GET_PARAM_1(0);
        sad_buff_append_str(server.output_buffer, thread_id_str);
    }

    if (strcmp(server.pkt_data.command, "qC") == 0) {
        char response[10];
        sprintf(response, "QC%d", server.builder.selected_core);
        sad_buff_append_str(server.output_buffer, response);
    }
    if (strcmp(server.pkt_data.command, "qSymbol") == 0) {
        sad_buff_append_str(server.output_buffer, "OK");
    } else
        build_unsupported();
}

// We're doing a simplification here
// in GDB if we do:
// 1. set scheduler-locking off (default) we're saying that when in the meantime
//	that a thread is executing a step or a continue, other threads can execute concurrently
// 2. set scheduler-locking on we're saying that when in the meantime
//	that a thread is executing a step or a continue it's the only one to effectively run
// 3. set scheduler-locking step it's just set scheduler-locking on but only in the step case
// in the continue case it's more like set-scheduler-off
//
// in this implementation we're assuming that everything is like "set scheduler-locking on"
// so only 1 thread at the time continue or steps, but eventually all thread can continue
// togheter if specified
// so for example if we receive "c" or "c:-1" we do continue all (like set scheduler-locking off)
// if we receive "c:0" we do continue only of thread 0, like set scheduler-locking on
// if we receive "s:0" we do only step of thread 0, like set scheduler-locking on
// if we receive "s" we assume that we step the selected core

static void build_v(void) {
    if (strcmp(server.pkt_data.command, "vCont?") == 0) {
        sad_buff_append_str(server.output_buffer, "s;c");
        return;

    } else if (strcmp(server.pkt_data.command, "vCont") == 0) {
        size_t fill = server.pkt_data.params_filled;
        int thread_id;
        const char *thread_id_str = NULL;

        if (fill == 0) {
            sad_buff_append_str(server.output_buffer, "E");
            return;
        }

        // s:#;c:#
        if (fill >= 2) {
            thread_id_str = GET_PARAM_1(1);
        }

        if (thread_id_str == NULL) {
            thread_id = (int) server.builder.selected_core;
        } else {
            long tmp_thread_id = strtol(thread_id_str, NULL, 16);
            LONG_TO_INT_CHECK(tmp_thread_id);
            thread_id = (int) tmp_thread_id;
        }

        if ((unsigned) thread_id >= server.sys_conf.smp) {
            sad_buff_append_str(server.output_buffer, "E");
            return;
        }

        // always check if there is some core on the breakpoint and correctly
        // exec the saved instruction before continuing

        switch (GET_PARAM_1(0)[0]) {
        case 'c':
            if ((thread_id_str == NULL) || (thread_id == -1)) {
                // this is the case of continue all so or just "c" or "c:-1"
                for (unsigned int i = 0; i < server.sys_conf.smp; i++)
                    sad_step_the_breakpoint(i);
                server.sys_ops.cores_continue();
            } else {
                sad_step_the_breakpoint((unsigned int) thread_id);
                server.sys_ops.core_continue((unsigned int) thread_id);
            }
            break;

        case 's':
            if ((thread_id_str == NULL) || (thread_id == -1)) {
                // this is the case of continue all so or just "s" or "s:-1"
                sad_buff_append_str(server.output_buffer, "E");
                return;
            } else {
                if (!sad_step_the_breakpoint((unsigned int) thread_id))
                    server.sys_ops.core_step((unsigned int) thread_id);
            }
            break;

        default:
            build_unsupported();
            return;
            break;
        }

        // wait that the cores hit a breakpoint or the user sends a stop signal
        // from GDB
        bool stop;
        stop = wait_for_halt((unsigned int) thread_id);
        if (stop) { // breakpoint stop or step stop
            if (GET_PARAM_1(0)[0] == 's')
                sad_buff_append_str(server.output_buffer, "S05");
            else
                sad_buff_append_str(server.output_buffer, "T05swbreak:;");
        } else // GDB interactive stop
            sad_buff_append_str(server.output_buffer, "S02");
    }
}

static void build_T(void) {
    sad_buff_append_str(server.output_buffer, "OK");
}

static void build_H(void) {
    if (strncmp(server.pkt_data.command, "Hg", 2) == 0) {
        const char *thread_id_str = server.pkt_data.command + 2;
        long thread_id = strtol(thread_id_str, NULL, 10);

        LONG_CHECK(thread_id);

        if (thread_id != -1) {
            LONG_TO_UINT(thread_id);
            server.builder.selected_core = (unsigned int) thread_id;
        }

        sad_buff_append_str(server.output_buffer, "OK");
    } else {
        build_unsupported();
    }
}

static void build_Z(void) {
    // format: Z type,addr,kind
    /**/
    const char *str_type = GET_CMD_PARAM;
    const char *str_addr = GET_PARAM_1(0);
    const char *str_kind = GET_PARAM_1(1);

    int type = atoi(str_type);
    int kind = atoi(str_kind);
    long addr = strtol(str_addr, NULL, 16);

    LONG_TO_UINT32_CHECK(addr);

    bool inserted;

    if (server.sys_conf.arch == RV32) {
        if ((type < 0) || (type > 4) || (kind != 4)) {
            // compressed (kind == 2) is unsupported atm
            sad_buff_append_str(server.output_buffer, "E");
            return;
        }

        inserted = sad_insert_breakpoint((uint32_t) addr);
        if (!inserted) {
            sad_buff_append_str(server.output_buffer, "E");
            return;
        }
    }

    sad_buff_append_str(server.output_buffer, "OK");
}

static void build_z(void) {
    // format: z type,addr,kind
    /**/
    const char *str_type = GET_CMD_PARAM;
    const char *str_addr = GET_PARAM_1(0);
    const char *str_kind = GET_PARAM_1(1);

    int type = atoi(str_type);
    int kind = atoi(str_kind);
    long addr = strtol(str_addr, NULL, 16);

    LONG_TO_UINT32_CHECK(addr);

    if (server.sys_conf.arch == RV32) {
        if ((type < 0) || (type > 4) || (kind != 4)) {
            // compressed (kind == 2) is unsupported atm
            sad_buff_append_str(server.output_buffer, "E");
            return;
        }

        sad_remove_breakpoint((uint32_t) addr);
    }

    sad_buff_append_str(server.output_buffer, "OK");
}

// SAD_EXTEND END

void sad_builder_build_resp(void) {
    uint8_t checksum;
    char hex_checksum[3];
    cmd_type cmd;

    if (server.ack_enabled)
        sad_buff_append_str(server.output_buffer, "+$");
    else
        sad_buff_append_str(server.output_buffer, "$");

    server.output_buffer->start_pkt_data = server.output_buffer->filled;

    // build resp
    cmd = supported_idx(server.pkt_data.command[0]);

    // call the correct builder
    server.builder.supported_builders[cmd]();

    server.output_buffer->end_pkt_data = server.output_buffer->filled;

    sad_buff_append_str(server.output_buffer, "#");

    checksum = sad_buff_checksum(server.output_buffer);
    sprintf(hex_checksum, "%02x", checksum);
    sad_buff_append_str(server.output_buffer, hex_checksum);
}
