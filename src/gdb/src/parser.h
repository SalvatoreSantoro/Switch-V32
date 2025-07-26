#ifndef _GDB_PARSER_H
#define _GDB_PARSER_H

#include "buffer.h"
#include "data.h"
#include "supported.h"
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

typedef void (*Parser_Fun)(Parser *);

struct Parser {
    Parser_Fun supported_parsers[COMMANDS_COUNT];
    pars_state state;
    pars_data_state data_state;
    size_t parse_idx;
    PKT_Buffer *buff;
};

void gdb_parser_init(Parser *parser, PKT_Buffer *buff);

void gdb_parser_reset(Parser* parser);

pars_state gdb_parser_pkt(Parser *parser, bool ack_enabled);

PKT_Data *gdb_parser_data(Parser *parser);

#endif
