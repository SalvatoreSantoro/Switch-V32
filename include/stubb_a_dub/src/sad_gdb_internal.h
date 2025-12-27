#ifndef SAD_GDB_INTERNAL_H
#define SAD_GDB_INTERNAL_H

#include "../stubb_a_dub.h"
#include "supported.h"
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// TYPES

typedef struct Parser Parser;
typedef struct Builder Builder;
typedef struct PKT_Data PKT_Data;
typedef struct PKT_Buffer PKT_Buffer;
typedef struct Breakpoint Breakpoint;

// TARGET
// target (arch) specific parameters
typedef struct {
    uint64_t breakpoint_val;
    uint32_t reg_bytes;
    uint32_t reg_str_bytes;
    uint32_t regs_bytes;
    uint32_t regs_str_bytes;
	uint32_t brkpt_size;
} Target_Conf;

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

typedef void (*Parse_Data_Fun)(char *);

struct Parser {
    pars_state state;
    size_t parse_idx;
    Parse_Data_Fun supported_parsers[COMMANDS_COUNT];
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
    uint64_t addr;
    uint64_t instr;
    breakpoint_status status;
};

// BUILDER

typedef void (*Builder_Fun)(void);

struct Builder {
    Builder_Fun supported_builders[COMMANDS_COUNT];
	// saved according to "user" most reasonable value...
	// so core 0 is 0, core 1 is 1 and so on...
	// gdb will take them with a +1 because it uses "0" as a special value
	// https://sourceware.org/gdb/current/onlinedocs/gdb.html/Packets.html#thread_002did-syntax
    unsigned int selected_core;
};

// UTIL

typedef enum {
    UTIL_OK,
    UTIL_UNEXPECTED,
    UTIL_X
} util_ret;

// BUFFER
struct PKT_Buffer {
    size_t initial_size;
    size_t start_pkt_data;
    size_t end_pkt_data;
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
    // sad client address
    struct sockaddr_in address;
    socklen_t addrlen;
} SAD_Stub;

// EXTRACT

// end of buff
#define EOB (-1)

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

void sad_parser_reset(void);

void sad_parser_pkt(void);

// BUFFER

PKT_Buffer *sad_buff_create(size_t initial_size);

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

brk_ret sad_insert_breakpoint(uint64_t addr);

brk_ret sad_remove_breakpoint(uint64_t addr);

Breakpoint *sad_find_breakpoint(uint64_t addr);

brk_ret sad_step_the_breakpoint(unsigned int core_idx);

void sad_breakpoint_reset(void);

// SOCKET

stub_ret sad_socket_init(uint16_t port);

int sad_socket_blocking(bool blocking);

// EXTRACT

stub_ret sad_extract_uint32(uint32_t *val, size_t *cursor, int term);

stub_ret sad_extract_int(int *val, size_t *cursor, int term);

stub_ret sad_extract_uint(unsigned int *val, size_t *cursor, int term);

stub_ret sad_extract_size(size_t *val, size_t *cursor, int term);

char *sad_extract_str(size_t *cursor, int term);

stub_ret sad_extract_uint64(uint64_t *val, size_t *cursor, int term);

stub_ret sad_extract_long(long *val, size_t *cursor, int term);

// TARGET

stub_ret sad_target_init(void);

void sad_target_read_regs(byte *regs, uint32_t core_id);

void sad_target_write_regs(const byte *regs, uint32_t core_id);

#endif
