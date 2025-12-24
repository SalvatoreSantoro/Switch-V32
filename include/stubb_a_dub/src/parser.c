#include "sad_gdb_internal.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern Parser parser_g;
extern PKT_Buffer* input_buffer_g;
extern bool ack_enabled_g;

void sad_parser_reset(void) {
    parser_g.state = PARSE_RESET;
    parser_g.parse_idx = 0;
}

void sad_parser_pkt(void) {
    uint8_t value;
    size_t buff_filled;
    static char checksum[3];
	checksum[2] = '\0';
    const byte *data = sad_buff_read_prep(input_buffer_g, &buff_filled);

    while (parser_g.parse_idx < buff_filled) {
        size_t idx = parser_g.parse_idx;

        switch (parser_g.state) {
        case PARSE_RESET:
            if (ack_enabled_g) {
                if (data[idx] == '+')
                    parser_g.state = PARSE_START;
                if (data[idx] == '-')
                    parser_g.state = PARSE_NACK;
            } else {
                parser_g.state = PARSE_START;
                // in case ack is disabled but still recived ack
                // need to don't skip iteration
                if (data[idx] != '+')
                    continue;
            }
            break;

        case PARSE_START:
            if (data[idx] == '$') {
                // first data byte
                input_buffer_g->start_pkt_data = idx + 1;
                parser_g.state = PARSE_SKIP;
            } else
                parser_g.state = PARSE_ERROR;
            break;

        case PARSE_SKIP:
            if (data[idx] == '#') { // last data byte
                input_buffer_g->end_pkt_data = idx;
                if (ack_enabled_g)
                    parser_g.state = PARSE_CHECKSUM_DIGIT_0;
                else
                    parser_g.state = PARSE_FINISHED;
            }
            break;

        case PARSE_CHECKSUM_DIGIT_0:
            checksum[0] = (char) data[idx];
            parser_g.state = PARSE_CHECKSUM_DIGIT_1;
            break;

        case PARSE_CHECKSUM_DIGIT_1:
            checksum[1] = (char) data[idx];
            // should always be 2 digits so from 00 to 99
            value = (uint8_t) strtol(checksum, NULL, 16);
            if (value != sad_buff_checksum(input_buffer_g))
                parser_g.state = PARSE_ERROR;
            else
                parser_g.state = PARSE_FINISHED;
            break;

        case PARSE_ERROR:
        case PARSE_NACK:
        case PARSE_FINISHED:
            break;
        default:
            assert(0 && "PARSER ERROR");
            break;
        }
        parser_g.parse_idx += 1;
    }
}
