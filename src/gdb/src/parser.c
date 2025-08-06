#include "parser.h"
#include "buffer.h"
#include "data.h"
#include <assert.h>
#include "defs.h"
#include <stdio.h>
#include <stdlib.h>


void sad_parser_init(Parser *parser, PKT_Buffer *buff) {
    // unchecked for convenience
    parser->pkt_data = sad_pkt_data_create(DEFAULT_PAR_SIZE);
    parser->pkt_buff = buff;
    sad_parser_reset(parser);
}

void sad_parser_reset(Parser *parser) {
    sad_pkt_data_reset(parser->pkt_data);
    sad_pkt_buff_reset(parser->pkt_buff);
    parser->state = PARSE_RESET;
    parser->data_state = DATA_RESET;
    parser->parse_idx = 0;
}

void sad_parser_deinit(Parser *parser) {
    sad_parser_reset(parser);
    sad_pkt_data_destroy(parser->pkt_data);
}

pars_state sad_parser_pkt(Parser *parser, bool ack_enabled) {
    uint8_t value;
    size_t buff_filled;
    char checksum[3];
    unsigned char *data = sad_buff_read_prep(&parser->pkt_buff->buff, &buff_filled);
    checksum[2] = '\0';

    while (parser->parse_idx < buff_filled) {
        size_t idx = parser->parse_idx;

        switch (parser->state) {
        case PARSE_RESET:
            if (ack_enabled) {
                if (data[idx] == '+')
                    parser->state = PARSE_START;
                if (data[idx] == '-')
                    parser->state = PARSE_NACK;
            } else {
                parser->state = PARSE_START;
                // in case ack is disabled but still recived ack
                // need to don't skip iteration
                if (data[idx] != '+')
                    continue;
            }
            break;

        case PARSE_START:
            if (data[idx] == '$') {
                // first data byte
                parser->pkt_buff->start_pkt_data = idx + 1;
                parser->state = PARSE_SKIP;
            } else
                parser->state = PARSE_ERROR;
            break;

        case PARSE_SKIP:
            if (data[idx] == '#') { // last data byte
                parser->pkt_buff->end_pkt_data = idx;
                if (ack_enabled)
                    parser->state = PARSE_CHECKSUM_DIGIT_0;
                else
                    parser->state = PARSE_FINISHED;
            }
            break;

        case PARSE_CHECKSUM_DIGIT_0:
            // printf("CHEK1 %c\n", data[idx]);
            checksum[0] = data[idx];
            parser->state = PARSE_CHECKSUM_DIGIT_1;
            break;

        case PARSE_CHECKSUM_DIGIT_1:
            // printf("CHEK2 %c\n", data[idx]);
            checksum[1] = data[idx];
            value = strtol(checksum, NULL, 16);
            if (value != sad_pkt_buff_checksum(parser->pkt_buff))
                parser->state = PARSE_ERROR;
            else
                parser->state = PARSE_FINISHED;
            break;

        case PARSE_ERROR:
        case PARSE_NACK:
        case PARSE_FINISHED:
            break;
        }
        parser->parse_idx += 1;
    }

    return parser->state;
}

PKT_Data *sad_parser_data(Parser *parser) {
    // check if the parser hasn't finished parsing packet structure
    // and initializing all the resources that this function assumes
    // to use
    if (parser->state != PARSE_FINISHED)
        return NULL;

    size_t idx;
    size_t end;
    unsigned char *data;
    char *prev_param = NULL;

    idx = parser->pkt_buff->start_pkt_data;
    end = parser->pkt_buff->end_pkt_data;

    data = sad_buff_read_prep(&parser->pkt_buff->buff, NULL);

    // command always starts as first byte of data
    parser->pkt_data->command = (char *) (data + idx);

    while (idx != end) {

        switch (data[idx]) {
        case ',':
        case ';':
        case ':':
            data[idx] = '\0'; // make the string so far an actual C string
            if (parser->data_state == DATA_WRITE_PARAM1) {
                sad_pkt_data_append_par(parser->pkt_data, prev_param, NULL);
                prev_param = (char *) data + idx + 1;
            } else { // DATA_RESET
                parser->data_state = DATA_WRITE_PARAM1;
                prev_param = (char *) data + idx + 1;
            }

            break;
        case '=':
            data[idx] = '\0';
            sad_pkt_data_append_par(parser->pkt_data, prev_param, (char *) (data + idx + 1));
            parser->data_state = DATA_RESET;
            break;
        }

        idx++;
    }

    data[idx] = '\0';
    if (parser->data_state == DATA_WRITE_PARAM1)
        sad_pkt_data_append_par(parser->pkt_data, prev_param, NULL);

    return parser->pkt_data;
}
