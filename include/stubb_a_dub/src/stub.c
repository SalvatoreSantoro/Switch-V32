#include "../stubb_a_dub.h"
#include "defs.h"
#include "sad_gdb_internal.h"
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

SAD_Stub server_g;
PKT_Buffer *input_buffer_g;
PKT_Buffer *output_buffer_g;
Parser parser_g;
Builder builder_g;
bool ack_enabled_g = true;
Sys_Ops sys_ops_g;
Sys_Conf sys_conf_g;
Breakpoint breakpoints_g[MAX_BREAKPOINTS];
Target_Conf target_conf_g;

static void sad_stub_reset_input_state(void) {
    sad_parser_reset();
    sad_buff_reset(input_buffer_g);
}

static void sad_stub_reset_output_state(void) {
    sad_buff_reset(output_buffer_g);
}

static void sad_stub_reset(void) {
    sad_stub_reset_input_state();
    sad_stub_reset_output_state();
}

// STUB
stub_ret sad_stub_init(Stub_Conf *conf) {
    stub_ret ret = STUB_OOM;

    // set to default
    size_t buff_size = conf->buffers_size > 0 ? conf->buffers_size : DEFAULT_BUFF_SIZE;
    uint16_t port = conf->port > 0 ? conf->port : DEFAULT_PORT;

    sys_conf_g = conf->sys_conf;
    sys_ops_g = conf->sys_ops;

    if (conf->sys_conf.arch != RV32)
        return STUB_ARCH;

    if (conf->sys_conf.smp > MAX_SMP)
        return STUB_SMP;

    // target init
    ret = sad_target_init();
    if (ret != STUB_OK)
        return STUB_UNEXPECTED;

    input_buffer_g = sad_buff_create(buff_size);
    if (input_buffer_g == NULL)
        goto input_err;

    output_buffer_g = sad_buff_create(buff_size);
    if (output_buffer_g == NULL)
        goto output_err;

    // builder init
    sad_builder_init();

    // reset state
    sad_stub_reset();

    // socket init
    ret = sad_socket_init(port);
    if (ret != STUB_OK)
        goto socket_err;

    // Reset breakpoints
    sad_breakpoint_reset();

    ack_enabled_g = true;
    return STUB_OK;

socket_err:
    sad_buff_destroy(output_buffer_g);
output_err:
    sad_buff_destroy(input_buffer_g);
input_err:
    return ret;
}

stub_ret sad_stub_handle_cmds(void) {
    buff_ret b_ret;

    // check that all cores are halted before running

    if (!sys_ops_g.is_halted())
        return STUB_HALTED;

    while (1) {
        b_ret = sad_buff_from_socket(input_buffer_g, server_g.sad_socket);
        if (b_ret == BUFF_OOM)
            return STUB_OOM;
        if (b_ret == BUFF_FD_ERR)
            return STUB_SOCKET;
        if (b_ret == BUFF_CLOSED)
            return STUB_CLOSED;

        // parse data just read in the input_buffer
        // the parser could be parsing an incomplete packet (without the ending "#")
        // so in case  sad_parser_pkt returns PARSING_INCOMPLETE the stub does nothing
        sad_parser_pkt();
#ifdef SAD_DEBUG
        sad_buff_print_content(input_buffer_g, "READ: ");
#endif

        if (parser_g.state == PARSE_ERROR) {
            // reset output in case last time it was "PARSING_GOT_NACK"
            sad_stub_reset_output_state();

            sad_buff_append_str(output_buffer_g, "-");

            if (sad_buff_to_socket(output_buffer_g, server_g.sad_socket) == BUFF_FD_ERR)
                return STUB_SOCKET;

            // reset everything for next iteration
            sad_stub_reset_input_state();
        }

        if (parser_g.state == PARSE_NACK) {
            // resend
            if (sad_buff_to_socket(output_buffer_g, server_g.sad_socket) == BUFF_FD_ERR)
                return STUB_SOCKET;

            // reset everything for next iteration
            sad_stub_reset_input_state();
        }

        if (parser_g.state == PARSE_FINISHED) {
            // reset output in case last time it was "PARSING_GOT_NACK"
            sad_stub_reset_output_state();

            sad_builder_build_resp();
#ifdef SAD_DEBUG
            sad_buff_print_content(output_buffer_g, "WRITE: ");
#endif

            if (sad_buff_to_socket(output_buffer_g, server_g.sad_socket) == BUFF_FD_ERR)
                return STUB_SOCKET;

            // reset everything for next iteration
            sad_stub_reset_input_state();
        }
    }
}

void sad_stub_deinit(void) {
    ack_enabled_g = true;
    close(server_g.server_fd);
    close(server_g.sad_socket);
    sad_buff_destroy(input_buffer_g);
    sad_buff_destroy(output_buffer_g);
}
