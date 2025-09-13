#define _GNU_SOURCE /* See feature_test_macros(7) */
#include "stub.h"
#include "data.h"
#include "parser.h"
#include "pkt_buffer.h"
#include <netinet/in.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef enum {
    STUB_RESET = 0,
    STUB_CONNECTED = 1,
    STUB_BRKPT = 2
} Stub_State;

typedef struct {
    int server_fd;
    int sad_socket;
    Stub_State state;
    // sad client address
    struct sockaddr_in address;
    socklen_t addrlen;
    int ack_enabled;
    PKT_Buffer *input_buffer;
    PKT_Buffer *output_buffer;
    Parser parser;
    Builder builder;
} sad_Stub;

static sad_Stub server = {.state = STUB_RESET};

// STUB
stub_ret sad_stub_init(int port, size_t buffers_size, size_t socket_io_size) {
    int opt = 1;
    struct sockaddr_in address;
    stub_ret ret = STUB_OOM;

    server.input_buffer = sad_pkt_buff_create(buffers_size, socket_io_size);
    if (server.input_buffer == NULL)
        goto input_err;

    server.output_buffer = sad_pkt_buff_create(buffers_size, socket_io_size);
    if (server.output_buffer == NULL)
        goto output_err;

    // parser on the input
    sad_parser_init(&server.parser, server.input_buffer);

    // builder on the output
    sad_builder_init(&server.builder, server.output_buffer);

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

    server.state = STUB_CONNECTED;
    server.ack_enabled = 1;
    return STUB_OK;

close_socket:
    close(server.server_fd);
socket_err:
    ret = STUB_SOCKET;
    sad_pkt_buff_destroy(server.output_buffer);
output_err:
    sad_pkt_buff_destroy(server.input_buffer);
input_err:
    return ret;
}

stub_ret sad_stub_handle_cmds(void) {
    pkt_buff_ret b_ret;
    pars_state p_state;
    PKT_Data *pkt_data;

    while (1) {
        b_ret = sad_pkt_buff_from_socket(server.input_buffer, server.sad_socket);
        if (b_ret == PKT_BUFF_OOM)
            return STUB_OOM;
        if (b_ret == PKT_BUFF_FD_ERR)
            return STUB_SOCKET;

        // parse data just read in the input_buffer
        // the parser could be parsing an incomplete packet (without the ending "#")
        // so in case  sad_parser_pkt returns PARSING_INCOMPLETE the stub does nothing
        p_state = sad_parser_pkt(&server.parser, server.ack_enabled);
        sad_pkt_buff_print_content(server.input_buffer, "READ: ");

        if (p_state == PARSE_ERROR) {
            // reset output in case last time it was "PARSING_GOT_NACK"
            sad_pkt_buff_reset(server.output_buffer);

            sad_pkt_buff_append_str(server.output_buffer, "-");

            if (sad_pkt_buff_to_socket(server.output_buffer, server.sad_socket) == PKT_BUFF_FD_ERR)
                return STUB_SOCKET;

            // reset everything for next iteration
            sad_parser_reset(&server.parser);
            sad_builder_reset(&server.builder);
        }

        if (p_state == PARSE_NACK) {
            // resend
            if (sad_pkt_buff_to_socket(server.output_buffer, server.sad_socket) == PKT_BUFF_FD_ERR)
                return STUB_SOCKET;

            // reset everything for next iteration
            sad_parser_reset(&server.parser);
            sad_builder_reset(&server.builder);
        }

        if (p_state == PARSE_FINISHED) {
            // reset output in case last time it was "PARSING_GOT_NACK"

            pkt_data = sad_parser_data(&server.parser);
            // sad_pkt_data_print(server.parser->pkt_data);

            sad_pkt_buff_reset(server.output_buffer);

            sad_builder_build_resp(&server.builder, pkt_data, server.ack_enabled);
            sad_pkt_buff_print_content(server.output_buffer, "WRITE: ");

            if (sad_pkt_buff_to_socket(server.output_buffer, server.sad_socket) == PKT_BUFF_FD_ERR)
                return STUB_SOCKET;

            // reset everything for next iteration
            sad_parser_reset(&server.parser);
            sad_builder_reset(&server.builder);
        }
    }
}

void sad_stub_reset(void) {
    if (server.state == STUB_RESET)
        return;

    server.state = STUB_RESET;
    server.ack_enabled = 1;
    close(server.server_fd);
    close(server.sad_socket);
    sad_pkt_buff_destroy(server.input_buffer);
    sad_pkt_buff_destroy(server.output_buffer);
    sad_parser_deinit(&server.parser);
    sad_builder_deinit(&server.builder);
}
