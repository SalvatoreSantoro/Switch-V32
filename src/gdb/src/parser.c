#include "parser.h"
#include "buffer.h"
#include "data.h"
#include "supported.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

// SAD_EXTEND START "Add new Parser function"
static void parse_q(Parser *parser);
static void parse_Q(Parser *parser);
static void parse_qst_mrk(Parser *parser);
static void parse_m(Parser *parser);
static void parse_M(Parser *parser);
// SAD_EXTEND END

void gdb_parser_init(Parser *parser, PKT_Buffer *buff) {
    parser->buff = buff;
    // SAD_EXTEND START "Init new Parser function"
    parser->supported_parsers[COMMAND_q] = parse_q;
    parser->supported_parsers[COMMAND_Q] = parse_Q;
    parser->supported_parsers[COMMAND_QSTMRK] = parse_qst_mrk;
    parser->supported_parsers[COMMAND_m] = parse_m;
    parser->supported_parsers[COMMAND_M] = parse_M;
    // SAD_EXTEND END

    gdb_parser_reset(parser);
}

void gdb_parser_reset(Parser *parser) {
    parser->state = PARSE_RESET;
    parser->data_state = DATA_RESET;
    parser->parse_idx = 0;
}

pars_state gdb_parser_pkt(Parser *parser, bool ack_enabled) {
    uint8_t value;
    size_t buff_filled;
    char checksum[3];
    unsigned char *data = gdb_buff_read_prep(parser->buff, &buff_filled);
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
                parser->buff->start_pkt_data = idx + 1;
                parser->state = PARSE_SKIP;
            } else
                parser->state = PARSE_ERROR;
            break;

        case PARSE_SKIP:
            if (data[idx] == '#') { // last data byte
                parser->buff->end_pkt_data = idx;
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
            if (value != gdb_buff_checksum(parser->buff))
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

PKT_Data *gdb_parser_data(Parser *parser) {
    // check if the parser hasn't finished parsing packet structure
    // and initializing all the resources that this function assumes
    // to use
    if (parser->state != PARSE_FINISHED)
        return NULL;
    CMD_Type cmd;

    // dispatch to correct parsing function using first char of message
    cmd = supported_idx(parser->buff->start_pkt_data);
    if (cmd == COMMAND_UNSUPPORTED)
        return NULL;
    parser->supported_parsers[cmd](parser);
}
