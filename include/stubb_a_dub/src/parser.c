#include "sad_gdb_internal.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern SAD_Stub server;

void sad_parser_reset(void) {
    server.parser.state = PARSE_RESET;
    server.parser.data_state = DATA_RESET;
    server.parser.parse_idx = 0;
}

void sad_parser_pkt(void) {
    uint8_t value;
    size_t buff_filled;
    char checksum[3];
    const byte *data = sad_buff_read_prep(server.input_buffer, &buff_filled);
    checksum[2] = '\0';

    while (server.parser.parse_idx < buff_filled) {
        size_t idx = server.parser.parse_idx;

        switch (server.parser.state) {
        case PARSE_RESET:
            if (server.ack_enabled) {
                if (data[idx] == '+')
                    server.parser.state = PARSE_START;
                if (data[idx] == '-')
                    server.parser.state = PARSE_NACK;
            } else {
                server.parser.state = PARSE_START;
                // in case ack is disabled but still recived ack
                // need to don't skip iteration
                if (data[idx] != '+')
                    continue;
            }
            break;

        case PARSE_START:
            if (data[idx] == '$') {
                // first data byte
                server.input_buffer->start_pkt_data = idx + 1;
                server.parser.state = PARSE_SKIP;
            } else
                server.parser.state = PARSE_ERROR;
            break;

        case PARSE_SKIP:
            if (data[idx] == '#') { // last data byte
                server.input_buffer->end_pkt_data = idx;
                if (server.ack_enabled)
                    server.parser.state = PARSE_CHECKSUM_DIGIT_0;
                else
                    server.parser.state = PARSE_FINISHED;
            }
            break;

        case PARSE_CHECKSUM_DIGIT_0:
            checksum[0] = (char) data[idx];
            server.parser.state = PARSE_CHECKSUM_DIGIT_1;
            break;

        case PARSE_CHECKSUM_DIGIT_1:
            checksum[1] = (char) data[idx];
			// should always be 2 digits so from 00 to 99
            value = (uint8_t) strtol(checksum, NULL, 16);
            if (value != sad_buff_checksum(server.input_buffer))
                server.parser.state = PARSE_ERROR;
            else
                server.parser.state = PARSE_FINISHED;
            break;

        case PARSE_ERROR:
        case PARSE_NACK:
        case PARSE_FINISHED:
            break;
        default:
            assert(0 && "PARSER ERROR");
            break;
        }
        server.parser.parse_idx += 1;
    }
}

void sad_parser_data(void) {
    // check if the parser hasn't finished parsing packet structure
    assert(server.parser.state == PARSE_FINISHED);

    size_t idx;
    size_t end;
    unsigned char *data;
    char *prev_param = NULL;

    idx = server.input_buffer->start_pkt_data;
    end = server.input_buffer->end_pkt_data;

    data = sad_buff_read_prep(server.input_buffer, NULL);

    // command always starts as first byte of data
    server.pkt_data.command = (char *) (data + idx);

    while (idx != end) {

        switch (data[idx]) {
        case ',':
        case ';':
        case ':':
            data[idx] = '\0'; // make the string so far an actual C string
            if (server.parser.data_state == DATA_WRITE_PARAM1) {
                sad_pkt_data_append_par(prev_param, NULL);
                prev_param = (char *) data + idx + 1;
            } else { // DATA_RESET
                server.parser.data_state = DATA_WRITE_PARAM1;
                prev_param = (char *) data + idx + 1;
            }

            break;
        case '=':
            data[idx] = '\0';
            sad_pkt_data_append_par(prev_param, (char *) (data + idx + 1));
            server.parser.data_state = DATA_RESET;
            break;
        default:
            break;
        }

        idx++;
    }

    data[idx] = '\0';
    if (server.parser.data_state == DATA_WRITE_PARAM1)
        sad_pkt_data_append_par(prev_param, NULL);
}
