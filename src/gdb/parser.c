#include "parser.h"
#include "buffer.h"
#include "data.h"
#include <stdlib.h>

#define DEF_PARAM_SIZE 2

Parser *gdb_parser_create(PKT_Buffer *buff) {
    Parser *parser;
    PKT_Data *pkt_data;

    if (buff == NULL)
        return NULL;

    pkt_data = gdb_pkt_data_create(DEF_PARAM_SIZE);
    if (pkt_data == NULL)
        return NULL;

    parser = malloc(sizeof(Parser));
    if (parser == NULL) {
        gdb_pkt_data_destroy(pkt_data);
        return NULL;
    }

    gdb_parser_reset(parser);
    parser->ack_activated = true;

    return parser;
}

void gdb_parser_destroy(Parser *parser) {
    gdb_pkt_data_destroy(parser->pkt_data);
    free(parser);
}

pars_ret gdb_parser_pkt(Parser *parser) {
    uint8_t value;
    size_t buff_filled;
    static char checksum[3];
    unsigned char *data = gdb_buff_read_prep(parser->buff, &buff_filled);
    checksum[2] = '\0';

    for (size_t idx = 0; idx < buff_filled; idx++) {
        switch (parser->state) {
        case PARSE_RESET:
            if (parser->ack_activated) {
                if (data[idx] == '+')
                    parser->state = PARSE_START;
                if (data[idx] == '-')
                    return PARSING_GOT_NACK;
            } else
                parser->state = PARSE_START;
            break;

        case PARSE_START:
            if (data[idx] == '$')
                parser->state = PARSE_SKIP;
            else
                return PARSING_GOT_ERROR;
            break;

        case PARSE_SKIP: // first data byte
            gdb_buff_set_start_pkt_data(parser->buff, idx);
            if (data[idx] == '#') { // last data byte
                gdb_buff_set_end_pkt_data(parser->buff, idx);
                if (parser->ack_activated)
                    parser->state = PARSE_CHECKSUM_DIGIT_0;
                else
                    return PARSING_HAS_FINISHED;
            }
            break;

        case PARSE_CHECKSUM_DIGIT_0:
            checksum[0] = data[idx];
            parser->state = PARSE_CHECKSUM_DIGIT_1;
            break;

        case PARSE_CHECKSUM_DIGIT_1:
            checksum[1] = data[idx];
            value = atoi(checksum);
            if (value != gdb_buff_checksum(parser->buff))
                return PARSING_GOT_ERROR;
            else
                return PARSING_HAS_FINISHED;
            break;
        }
    }

    return PARSING_INCOMPLETE;
}
