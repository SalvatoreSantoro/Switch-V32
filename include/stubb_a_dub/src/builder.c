#include "../stubb_a_dub.h"
#include "sad_gdb_internal.h"
#include "supported.h"
#include <assert.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <sched.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern PKT_Buffer *input_buffer_g;
extern PKT_Buffer *output_buffer_g;
extern SAD_Stub server_g;
extern bool ack_enabled_g;
extern Sys_Ops sys_ops_g;
extern Sys_Conf sys_conf_g;
extern Builder builder_g;
extern Target_Conf target_conf_g;

// define builder functions
#define X(s, ch) static void build_##s(void);
SUPPORTED_CMDS
#undef X

// a bit of an hack to avoid a milion error checkings...
// forgive me for using macros like these :(

#define extract_val(type, addr, cursor, term)                                                                          \
    do {                                                                                                               \
        stub_ret ret = sad_extract_##type((addr), (cursor), (term));                                                   \
        if (ret != STUB_OK) {                                                                                          \
            sad_buff_append_str(output_buffer_g, "E");                                                                 \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0);

static bool wait_for_halt(void) {
    // set the socket as non blocking
    bool breakp_stop = false;
    size_t filled;
    const unsigned char *data;
    sad_socket_blocking(false);

    while (!breakp_stop) {
        // check GDB stop message
        sad_buff_from_socket(input_buffer_g, server_g.sad_socket);

        // ^C from gdb
        data = sad_buff_read_prep(input_buffer_g, &filled);

        if (data[filled - 1] == '\x03') {
            sys_ops_g.cores_halt();
            break;
        }

        // check if core halted
        // just check the core_idx because as a convention a breakpoint stops all the threads
        // nonblocking
        breakp_stop = sys_ops_g.is_halted();

        if (!breakp_stop)
            sched_yield();
    }
    // reset to blocking socket
    sad_socket_blocking(true);
    return breakp_stop;
}

void sad_builder_init(void) {
    builder_g.selected_core = 0;

#define X(s, ch) builder_g.supported_builders[COMMAND_##s] = build_##s;
    SUPPORTED_CMDS
#undef X
}

static void build_unsupported(void) {
    return;
}

static void build_p(void) {
    // format: p n
    size_t cursor = input_buffer_g->start_pkt_data + 1;
    uint32_t reg_id;
    uint64_t reg;
    extract_val(uint32, &reg_id, &cursor, EOB);

    if (reg_id >= sys_conf_g.regs_num) {
        sad_buff_append_str(output_buffer_g, "E");
        return;
    }

    char reg_str[target_conf_g.reg_str_bytes];

    reg = sys_ops_g.read_reg(builder_g.selected_core, reg_id);
    sad_bytes_to_hex_chars(reg_str, (byte *) &reg, sizeof(reg_str), target_conf_g.reg_bytes);

    sad_buff_append(output_buffer_g, reg_str, sizeof(reg_str));
}

static void build_P(void) {
    // format: P n=r
    size_t cursor = input_buffer_g->start_pkt_data + 1;
    uint32_t reg_id;
    uint64_t reg;
    extract_val(uint32, &reg_id, &cursor, '=');
    extract_val(uint64, &reg, &cursor, EOB);

    if (reg_id >= sys_conf_g.regs_num) {
        sad_buff_append_str(output_buffer_g, "E");
        return;
    }

    sys_ops_g.write_reg(reg, builder_g.selected_core, reg_id);

    sad_buff_append_str(output_buffer_g, "OK");
}

static void build_g(void) {
    byte regs_bytes[target_conf_g.regs_bytes];
    char regs_chars[target_conf_g.regs_str_bytes];

    sad_target_read_regs(regs_bytes, builder_g.selected_core);
    // Convert to string;
    assert((sizeof(regs_bytes) * 2) == sizeof(regs_chars));
    sad_bytes_to_hex_chars(regs_chars, regs_bytes, sizeof(regs_chars), sizeof(regs_bytes));
    // Reply
    sad_buff_append(output_buffer_g, regs_chars, sizeof(regs_chars));
}

static void build_G(void) {
    // format: G XX...

    util_ret ret;
    uint64_t reg;
    byte old_regs[target_conf_g.regs_bytes];
    size_t cursor = input_buffer_g->start_pkt_data + 1;
    uint32_t index = 0;

    char temp[9] = {0};

    const char *regs_str = sad_extract_str(&cursor, EOB);

    if (strlen(regs_str) != target_conf_g.regs_str_bytes) {
        sad_buff_append_str(output_buffer_g, "E");
        return;
    }

    sad_target_read_regs(old_regs, builder_g.selected_core);

    // iterate every register
    // overwriting the old registers content
    for (uint32_t i = 0; i < sys_conf_g.regs_num; i++) {
        ret = sad_hex_chars_to_bytes((byte *) &reg, regs_str + index, target_conf_g.reg_bytes,
                                     target_conf_g.reg_str_bytes);

        memcpy(temp, regs_str + index, 8);

        index += target_conf_g.reg_str_bytes;
        // when reading "XXXXXXXX" skip register
        if (ret == UTIL_X)
            continue;

        memcpy(old_regs + (i * target_conf_g.reg_bytes), &reg, target_conf_g.reg_bytes);
    }

    sad_target_write_regs(old_regs, builder_g.selected_core);

    sad_buff_append_str(output_buffer_g, "OK");
}

static void build_m(void) {
    // format: m addr,length

    // skip "m" character
    size_t cursor = input_buffer_g->start_pkt_data + 1;
    uint64_t addr;
    uint64_t length;
    uint64_t output_sz;

    extract_val(uint64, &addr, &cursor, ',');
    extract_val(uint64, &length, &cursor, EOB);

    if ((addr + length) >= sys_conf_g.memory_size) {
        sad_buff_append_str(output_buffer_g, "E");
        return;
    }

    output_sz = length * 2;

    char output[output_sz];
    byte mem[length];

    sys_ops_g.read_mem(mem, length, addr);

    sad_bytes_to_hex_chars(output, mem, output_sz, length);
    sad_buff_append(output_buffer_g, output, output_sz);
}

static void build_M(void) {
    // formt: M addr,length:XX

    // skip "M" character
    size_t cursor = input_buffer_g->start_pkt_data + 1;
    uint64_t addr;
    uint64_t length;

    extract_val(uint64, &addr, &cursor, ',');
    extract_val(uint64, &length, &cursor, ':');

    if ((addr + length) >= sys_conf_g.memory_size) {
        sad_buff_append_str(output_buffer_g, "E");
        return;
    }

    const char *mem_str = sad_extract_str(&cursor, EOB);

    if (strlen(mem_str) != (length * 2)) {
        sad_buff_append_str(output_buffer_g, "E");
        return;
    }

    byte data[length];

    // prepare data to write
    sad_hex_str_to_bytes(data, mem_str, length);

    sys_ops_g.write_mem(data, length, addr);

    sad_buff_append_str(output_buffer_g, "OK");
}

static void build_qstmrk(void) {
    sad_buff_append_str(output_buffer_g, "S05");
}

static void build_Q(void) {
    size_t cursor = input_buffer_g->start_pkt_data;
    const char *command_str = sad_extract_str(&cursor, ':');
    if (command_str == NULL) {
        sad_buff_append_str(output_buffer_g, "E");
        return;
    }

    if (strcmp(command_str, "QStartNoAckMode") == 0) {
        ack_enabled_g = false;
        sad_buff_append_str(output_buffer_g, "OK");
    }
}

static void build_q(void) {
    size_t cursor = input_buffer_g->start_pkt_data;
    size_t cursor_extra = input_buffer_g->start_pkt_data;
    char *command = sad_extract_str(&cursor, ':');
    char *command_extra = sad_extract_str(&cursor_extra, ',');
    char core_id_str[16];
    char response[10];
    const char *core_name;
    uint32_t thread_id;
    unsigned int i;

    //  format ‘qThreadExtraInfo,thread-id’...
    //  is different from some reasons...
    if (strncmp(command_extra, "qThreadExtraInfo", 16) == 0) {
        extract_val(uint32, &thread_id, &cursor_extra, EOB);
        if ((thread_id - 1) >= sys_conf_g.smp) {
            sad_buff_append_str(output_buffer_g, "E");
            return;
        }
        if (thread_id == 0) {
            core_name = sys_ops_g.core_name(builder_g.selected_core);
        } else {
            core_name = sys_ops_g.core_name(thread_id - 1);
        }

        byte core_name_hex[strlen(core_name) * 2];

        sad_bytes_to_hex_chars((char *) core_name_hex, (const byte *) core_name, sizeof(core_name_hex),
                               strlen(core_name));

        sad_buff_append(output_buffer_g, (char *) core_name_hex, sizeof(core_name_hex));
    }

    if (strcmp(command, "qSupported") == 0) {
        sad_buff_append_str(output_buffer_g, "swbreak+;vCont+;QStartNoAckMode+");
    }

    if (strcmp(command, "qfThreadInfo") == 0) {
        sad_buff_append_str(output_buffer_g, "m");
        for (i = 1; i < sys_conf_g.smp; i++) {
            sprintf(core_id_str, "%u,", i);
            sad_buff_append_str(output_buffer_g, core_id_str);
        }
        sprintf(core_id_str, "%u", i);
        sad_buff_append_str(output_buffer_g, core_id_str);
    }
    if (strcmp(command, "qsThreadInfo") == 0) {
        sad_buff_append_str(output_buffer_g, "l");
    }

    if (strcmp(command, "qC") == 0) {
        sprintf(response, "QC%u", builder_g.selected_core + 1);
        sad_buff_append_str(output_buffer_g, response);
    }

    if (strcmp(command, "qSymbol") == 0) {
        sad_buff_append_str(output_buffer_g, "OK");
    }
}

// We're doing a simplification here
// in GDB if we do:
// 1. set scheduler-locking off (default) we're saying that when in the meantime
//	that a thread is executing a step or a continue, other threads can execute concurrently
// 2. set scheduler-locking on we're saying that when in the meantime
//	that a thread is executing a step or a continue it's the only one to effectively run
// 3. set scheduler-locking step is just set scheduler-locking on but only in the step case
// in the continue case it's more like set-scheduler-off
//
// in this implementation we're assuming that everything is like "set scheduler-locking on"
// so only 1 thread at the time continue or steps, but eventually all thread can continue
// togheter if specified
// so for example if we receive "c" or "c:-1" we do continue all (like set scheduler-locking off)
// if we receive "c:1" we do continue only of thread 1, like set scheduler-locking on
// if we receive "s:1" we do only step of thread 1, like set scheduler-locking on
// if we receive "s" we throw an error (because missing id means every core)

static void build_v(void) {
    size_t cursor = input_buffer_g->start_pkt_data;
    brk_ret brk_ret;
    const char *command = sad_extract_str(&cursor, ';');

    if (strcmp(command, "vCont?") == 0) {
        // only step and continue supported
        sad_buff_append_str(output_buffer_g, "s;c");
        return;

    } else if (strcmp(command, "vCont") == 0) {
        // evaluating just the left most action
        char action = (char) input_buffer_g->data[cursor];
        long thread_id;
        long halter_id;
        char halter_id_str[16] = {0};
        bool stop;
        uint64_t reg;
        char reg_str[target_conf_g.reg_str_bytes];
        char reg_id_str[16] = {0};

        // the ":id" part is optional so check, if there isn't ":" the id is omitted, just assume -1
        if (input_buffer_g->data[cursor + 1] == ':') {
            // skip "action" and ":" already fetched
            cursor += 2;
            // this will place the "\0" after the id to make thread_id_str effectively a string
            extract_val(long, &thread_id, &cursor, ':');
        } else {
            thread_id = -1;
        }

        if ((thread_id - 1 >= sys_conf_g.smp) || (thread_id < -1)) {
            sad_buff_append_str(output_buffer_g, "E");
            return;
        }

        if (thread_id == 0)
            thread_id = builder_g.selected_core;

        if (thread_id > 0)
            thread_id--;

        switch (action) {
        case 'c':
            if (thread_id == -1) {
                // this is the case of continue all so or just "c" or "c:-1"
                for (unsigned int i = 0; i < sys_conf_g.smp; i++)
                    sad_step_the_breakpoint(i);

                sys_ops_g.cores_continue();
            } else {
                sad_step_the_breakpoint((uint32_t) thread_id);
                sys_ops_g.core_continue((uint32_t) thread_id);
            }
            break;

        case 's':
            // can only step 1 thread
            if (thread_id == -1) {
                sad_buff_append_str(output_buffer_g, "E");
                return;
            }

            brk_ret = sad_step_the_breakpoint((uint32_t) thread_id);
            // the current core isn't stopped on a breakpoint so just step it directly
            if (brk_ret == BRK_NOT_FOUND)
                sys_ops_g.core_step((uint32_t) thread_id);

            break;
        default:
            sad_buff_append_str(output_buffer_g, "E");
            return;
            break;
        }

        // wait that the cores hit a breakpoint or the user sends a stop signal
        // from GDB
        stop = wait_for_halt();

        if (!stop) {
            // GDB interactive stop
            sad_buff_append_str(output_buffer_g, "S02");
            return;
        }

        // breakpoint stop or step stop
        sad_buff_append_str(output_buffer_g, "T05swbreak:;");

        if (thread_id == -1) {
            halter_id = sad_breakpoint_hartid();
            // not found
            if (halter_id == -1)
                return;
        } else {
            halter_id = thread_id;
        }

        // gdb wants id from 1 not 0...
        sprintf(halter_id_str, "thread:%d;", (uint32_t) halter_id + 1);
        sad_buff_append_str(output_buffer_g, halter_id_str);
        sprintf(halter_id_str, "core:%d;", (uint32_t) halter_id + 1);
        sad_buff_append_str(output_buffer_g, halter_id_str);

        // read each register and place it like a string
        for (uint32_t reg_id = 0; reg_id < sys_conf_g.regs_num; reg_id++) {
            reg = sys_ops_g.read_reg((uint32_t) halter_id, reg_id);
            sad_bytes_to_hex_chars(reg_str, (byte *) &reg, sizeof(reg_str), target_conf_g.reg_bytes);
            sprintf(reg_id_str, "%02x:", reg_id);
            sad_buff_append_str(output_buffer_g, reg_id_str);
            sad_buff_append(output_buffer_g, reg_str, sizeof(reg_str));
            sad_buff_append_str(output_buffer_g, ";");
        }
    }
}

static void build_T(void) {
    sad_buff_append_str(output_buffer_g, "OK");
}

static void build_H(void) {
    size_t cursor = input_buffer_g->start_pkt_data;
    char *command = sad_extract_str(&cursor, EOB);
    uint32_t thread_id;
    if (strncmp(command, "Hg", 2) == 0) {
        cursor = input_buffer_g->start_pkt_data + 2;
        extract_val(uint32, &thread_id, &cursor, EOB);

        // select any, we still use the "0" core
        if (thread_id == 0)
            builder_g.selected_core = thread_id;
        else if ((thread_id - 1) >= sys_conf_g.smp) {
            sad_buff_append_str(output_buffer_g, "E");
            return;
        } else {
            builder_g.selected_core = thread_id - 1;
        }

        sad_buff_append_str(output_buffer_g, "OK");
    }
}

static void build_Z(void) {
    // format: Z type,addr,kind
    // ignoring the optional params

    size_t cursor = input_buffer_g->start_pkt_data + 1;
    uint32_t type;
    uint32_t kind;
    uint64_t addr;
    extract_val(uint32, &type, &cursor, ',');
    extract_val(uint64, &addr, &cursor, ',');
    extract_val(uint32, &kind, &cursor, ';');

    if (addr >= sys_conf_g.memory_size) {
        sad_buff_append_str(output_buffer_g, "E");
        return;
    }

    if (kind != target_conf_g.brkpt_size) {
        sad_buff_append_str(output_buffer_g, "E");
        return;
    }

    // only sw breakpoint supported
    if (type != 0) {
        sad_buff_append_str(output_buffer_g, "E");
        return;
    }

    if (sad_insert_breakpoint(addr) != BRK_INSERTED) {
        sad_buff_append_str(output_buffer_g, "E");
        return;
    }

    sad_buff_append_str(output_buffer_g, "OK");
}

static void build_z(void) {
    // format: z type,addr,kind

    size_t cursor = input_buffer_g->start_pkt_data + 1;
    uint32_t type;
    uint32_t kind;
    uint64_t addr;
    extract_val(uint32, &type, &cursor, ',');
    extract_val(uint64, &addr, &cursor, ',');
    extract_val(uint32, &kind, &cursor, EOB);

    if (addr >= sys_conf_g.memory_size) {
        sad_buff_append_str(output_buffer_g, "E");
        return;
    }

    if (kind != target_conf_g.brkpt_size) {
        sad_buff_append_str(output_buffer_g, "E");
        return;
    }

    // only sw breakpoint supported
    if (type != 0) {
        sad_buff_append_str(output_buffer_g, "E");
        return;
    }

    if (sad_remove_breakpoint(addr) != BRK_REMOVED) {
        sad_buff_append_str(output_buffer_g, "E");
        return;
    }

    sad_buff_append_str(output_buffer_g, "OK");
}

void sad_builder_build_resp(void) {
    uint8_t checksum;
    char hex_checksum[3];
    cmd_type cmd_idx;
    char data_str = (char) *(input_buffer_g->data + input_buffer_g->start_pkt_data);

    if (ack_enabled_g)
        sad_buff_append_str(output_buffer_g, "+$");
    else
        sad_buff_append_str(output_buffer_g, "$");

    output_buffer_g->start_pkt_data = output_buffer_g->filled;

    // build resp
    cmd_idx = supported_idx(data_str);

    // call the correct builder
    builder_g.supported_builders[cmd_idx]();

    output_buffer_g->end_pkt_data = output_buffer_g->filled;

    sad_buff_append_str(output_buffer_g, "#");

    checksum = sad_buff_checksum(output_buffer_g);

    sprintf(hex_checksum, "%02x", checksum);
    sad_buff_append_str(output_buffer_g, hex_checksum);
}
