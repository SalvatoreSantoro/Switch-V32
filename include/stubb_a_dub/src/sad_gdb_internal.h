#ifndef SAD_GDB_INTERNAL_H
#define SAD_GDB_INTERNAL_H

#include "../stubb_a_dub.h"
#include "defs.h"
#include "supported.h"
#include <netinet/in.h>
#include <stdint.h>

// TYPES

typedef struct Parser Parser;
typedef struct Builder Builder;
typedef struct PKT_Data PKT_Data;
typedef struct PKT_Buffer PKT_Buffer;
typedef struct Breakpoint Breakpoint;

// PKT_DATA

typedef struct {
    char *param1;
    char *param2; // Empty string for key-only/value params
} Data_Param;

struct PKT_Data {
    size_t params_sz;
    size_t params_filled;
    char *command;
    Data_Param *params;
};

typedef enum {
    DATA_OK,
    DATA_OOM,
} data_ret;

// PARSER
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

struct Parser {
    pars_state state;
    pars_data_state data_state;
    size_t parse_idx;
};

// BREAKPOINT

typedef enum {
    BRK_EMPTY,
    BRK_FULL
} breakpoint_status;

typedef enum {
    BRK_ERROR,
    BRK_INSERTED,
    BRK_REMOVED,
    BRK_STEPPED,
    BRK_NOT_FOUND
} brk_ret;

struct Breakpoint {
    uint32_t addr;
    uint32_t instr;
    breakpoint_status status;
};

// BUILDER

typedef void (*Builder_Fun)(void);

struct Builder {
    Builder_Fun supported_builders[COMMANDS_COUNT];
    size_t cached_regs_bytes;
    size_t cached_regs_str_bytes;
    unsigned int selected_core;
};

// BUFFER
struct PKT_Buffer {
    size_t initial_size;
    size_t start_pkt_data;
    size_t end_pkt_data;
    size_t socket_io_size;
    size_t filled;
    size_t data_size;
    byte *data;
};

typedef enum {
    BUFF_OK,
    BUFF_OOM,
    BUFF_FD_ERR,
    BUFF_WOULD_BLOCK,
    BUFF_CLOSED
} buff_ret;

// STUB

typedef struct {
    int server_fd;
    int sad_socket;
    bool ack_enabled;
    PKT_Buffer *input_buffer;
    PKT_Buffer *output_buffer;
    PKT_Data pkt_data;
    Parser parser;
    Builder builder;
    Sys_Ops sys_ops;
    Sys_Conf sys_conf;
    // sad client address
    struct sockaddr_in address;
    socklen_t addrlen;
    Breakpoint breakpoints[MAX_BREAKPOINTS];
} SAD_Stub;

// UTILS

typedef enum {
    UTIL_SIZE_ERROR,
    UTIL_NO_HEX,
    UTIL_OK
} util_ret;

// Functions

// UTILS
util_ret sad_hex_chars_to_bytes(byte *dest, const char *src, size_t dest_size, size_t src_size);

util_ret sad_hex_str_to_bytes(byte *dest, const char *src, size_t dest_size);

util_ret sad_bytes_to_hex_chars(char *dest, const byte *src, size_t dest_size, size_t src_size);

// BUILDER
void sad_builder_init(void);

void sad_builder_build_resp(void);

// PARSER

void sad_parser_reset(void);

void sad_parser_pkt(void);

void sad_parser_data(void);

// PKT DATA
data_ret sad_pkt_data_init(void);

void sad_pkt_data_deinit(void);

void sad_pkt_data_reset(void);

data_ret sad_pkt_data_append_par(char *param1, char *param2);

void sad_pkt_data_print(void);

Data_Param *sad_pkt_data_find(PKT_Data *pkt_data, const char *param1);

// BUFFER

PKT_Buffer *sad_buff_create(size_t initial_size, size_t socket_io_size);

void sad_buff_destroy(PKT_Buffer *buff);

// returns -1 on error, 0 otherwise
buff_ret sad_buff_expand(PKT_Buffer *buff);

// returns -1 on error, 0 otherwise
buff_ret sad_buff_append(PKT_Buffer *buff, const char *data, size_t data_size);

unsigned char *sad_buff_read_prep(const PKT_Buffer *buff, size_t *buff_filled);

buff_ret sad_buff_from_socket(PKT_Buffer *buff, int fd);

buff_ret sad_buff_to_socket(PKT_Buffer *buff, int fd);

void sad_buff_print_content(const PKT_Buffer *buff, const char *str);

buff_ret sad_buff_append_str(PKT_Buffer *buff, const char *str);

uint8_t sad_buff_checksum(PKT_Buffer *buff);

void sad_buff_reset(PKT_Buffer *buff);

// BREAKPOINT

brk_ret sad_insert_breakpoint(uint32_t addr);

brk_ret sad_remove_breakpoint(uint32_t addr);

Breakpoint *sad_find_breakpoint(uint32_t addr);

brk_ret sad_step_the_breakpoint(unsigned int core_idx);

#endif
