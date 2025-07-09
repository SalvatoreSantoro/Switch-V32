#ifndef _GDB_PARSER_H
#define _GDB_PARSER_H

#include "buffer.h"
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    PARSE_RESET,
    PARSE_START,
    PARSE_SKIP,
    PARSE_CHECKSUM_DIGIT_0,
    PARSE_CHECKSUM_DIGIT_1
} pars_state;

typedef enum {
    PARSING_HAS_FINISHED,
    PARSING_INCOMPLETE,
    PARSING_GOT_ERROR,
    PARSING_GOT_NACK // resend the last message
} pars_ret;

typedef struct {
    pars_state state;
    size_t parse_idx;
    bool ack_activated;
    PKT_Data *pkt_data;
    PKT_Buffer *buff;
} Parser;

Parser *gdb_parser_create(PKT_Buffer *buff);

void gdb_parser_destroy(Parser *parser);

pars_ret gdb_parser_pkt(Parser *parser);

void gdb_parser_data(Parser *parser);

#define gdb_parser_reset(parser)                                                                                       \
    do {                                                                                                               \
        parser->state = PARSE_RESET;                                                                                   \
        parser->parse_idx = 0;                                                                                         \
        gdb_pkt_data_reset(parser->pkt_data);                                                                          \
    } while (0)

#endif
