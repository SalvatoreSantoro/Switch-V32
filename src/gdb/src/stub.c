#include "defs.h"
#include "sad_gdb.h"
#include "sad_gdb_internal.h"
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

SAD_Stub server;

static void sad_stub_reset_input_state() {
    sad_parser_reset();
    sad_pkt_data_reset();
    sad_buff_reset(server.input_buffer);
}

static void sad_stub_reset_output_state() {
    sad_buff_reset(server.output_buffer);
}

static void sad_stub_reset() {
    sad_stub_reset_input_state();
    sad_stub_reset_output_state();
}

// STUB
stub_ret sad_stub_init(Stub_Conf *conf) {
    int opt = 1;
    struct sockaddr_in address;
    stub_ret ret = STUB_OOM;

    // set to default
    size_t buff_size = conf->buffers_size > 0 ? conf->buffers_size : DEFAULT_BUFF_SIZE;
    size_t socket_io_size = conf->socket_io_size > 0 ? conf->socket_io_size : DEFAULT_SOCKET_IO_SIZE;
    int port = conf->port > 0 ? conf->port : DEFAULT_PORT;

    server.sys_conf = conf->sys_conf;
    server.sys_ops = conf->sys_ops;

    if (conf->sys_conf.arch != RV32)
        return STUB_ARCH;

    if ((conf->sys_conf.smp <= 0) || (conf->sys_conf.smp > MAX_SMP))
        return STUB_SMP;

    server.input_buffer = sad_buff_create(buff_size, socket_io_size);
    if (server.input_buffer == NULL)
        goto input_err;

    server.output_buffer = sad_buff_create(buff_size, socket_io_size);
    if (server.output_buffer == NULL)
        goto output_err;

    // create pkt data
    if (sad_pkt_data_init() == DATA_OOM)
        return STUB_OOM;

    // builder init
    sad_builder_init();

    // reset state
    sad_stub_reset();

    if ((server.server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        goto socket_err;

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (setsockopt(server.server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        goto close_socket;

    if (bind(server.server_fd, (struct sockaddr *) &address, sizeof(address)) == -1)
        goto close_socket;

    // set backlog to 1 (hint on number of outstanding connections
    // in the socket's listen queue)
    if (listen(server.server_fd, 1) == -1)
        goto close_socket;

    server.sad_socket = accept(server.server_fd, (struct sockaddr *) &server.address, &server.addrlen);
    if (server.sad_socket == -1)
        goto close_socket;

	// Reset breakpoints
    for (size_t i = 0; i < MAX_BREAKPOINTS ; i++)
        server.breakpoints[i].status = BRK_EMPTY;

    server.ack_enabled = true;
    return STUB_OK;

close_socket:
    close(server.server_fd);
socket_err:
    ret = STUB_SOCKET;
    sad_buff_destroy(server.output_buffer);
output_err:
    sad_buff_destroy(server.input_buffer);
input_err:
    return ret;
}

stub_ret sad_stub_handle_cmds(void) {
    buff_ret b_ret;

    while (1) {
        b_ret = sad_buff_from_socket(server.input_buffer, server.sad_socket);
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
        sad_buff_print_content(server.input_buffer, "READ: ");
#endif

        if (server.parser.state == PARSE_ERROR) {
            // reset output in case last time it was "PARSING_GOT_NACK"
            sad_stub_reset_output_state();

            sad_buff_append_str(server.output_buffer, "-");

            if (sad_buff_to_socket(server.output_buffer, server.sad_socket) == BUFF_FD_ERR)
                return STUB_SOCKET;

            // reset everything for next iteration
            sad_stub_reset_input_state();
        }

        if (server.parser.state == PARSE_NACK) {
            // resend
            if (sad_buff_to_socket(server.output_buffer, server.sad_socket) == BUFF_FD_ERR)
                return STUB_SOCKET;

            // reset everything for next iteration
            sad_stub_reset_input_state();
        }

        if (server.parser.state == PARSE_FINISHED) {
            // reset output in case last time it was "PARSING_GOT_NACK"
            sad_stub_reset_output_state();

            // parse data to initialize server PKT_Data
            sad_parser_data();

            // sad_pkt_data_print();

            sad_builder_build_resp();
#ifdef SAD_DEBUG
            sad_buff_print_content(server.output_buffer, "WRITE: ");
#endif

            if (sad_buff_to_socket(server.output_buffer, server.sad_socket) == BUFF_FD_ERR)
                return STUB_SOCKET;

            // reset everything for next iteration
            sad_stub_reset_input_state();
        }
    }
}

void sad_stub_deinit(void) {
    server.ack_enabled = true;
    close(server.server_fd);
    close(server.sad_socket);
    sad_buff_destroy(server.input_buffer);
    sad_buff_destroy(server.output_buffer);
    sad_pkt_data_deinit();
}
