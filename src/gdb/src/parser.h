#ifndef _GDB_PARSER_H
#define _GDB_PARSER_H

#include "buffer.h"
#include "data.h"
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


typedef struct {
    pars_state state;
    pars_data_state data_state;
    size_t parse_idx;
    PKT_Data *pkt_data;
    PKT_Buffer *buff;
} Parser;

Parser *gdb_parser_create(PKT_Buffer *buff);

void gdb_parser_destroy(Parser *parser);

pars_state gdb_parser_pkt(Parser *parser, bool ack_enabled);

PKT_Data *gdb_parser_data(Parser *parser);

#define gdb_parser_reset(parser)                                                                                       \
    do {                                                                                                               \
        parser->state = PARSE_RESET;                                                                                   \
        parser->data_state = DATA_RESET;                                                                               \
        parser->parse_idx = 0;                                                                                         \
        gdb_pkt_data_reset(parser->pkt_data);                                                                          \
    } while (0)

#endif
