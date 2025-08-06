#ifndef PARSER_H
#define PARSER_H

#include "data.h"
#include "pkt_buffer.h"
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    PARSE_RESET,
    PARSE_START,
    PARSE_SKIP,
    PARSE_CHECKSUM_DIGIT_0,
    PARSE_CHECKSUM_DIGIT_1,
    PARSE_NACK,
    PARSE_ERROR,
    PARSE_FINISHED
} pars_state;

typedef enum {
    DATA_RESET,
    DATA_WRITE_PARAM1,
} pars_data_state;

typedef struct Parser Parser;

struct Parser {
    pars_state state;
    pars_data_state data_state;
    size_t parse_idx;
    PKT_Data *pkt_data;
    PKT_Buffer *pkt_buff;
};

void sad_parser_init(Parser *parser, PKT_Buffer *buff);

void sad_parser_deinit(Parser *parser);

void sad_parser_reset(Parser *parser);

pars_state sad_parser_pkt(Parser *parser, bool ack_enabled);

PKT_Data *sad_parser_data(Parser *parser);

#endif
