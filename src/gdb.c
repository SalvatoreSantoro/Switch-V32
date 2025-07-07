#include "gdb.h"
#include "defs.h"
#include <netinet/in.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define GDB_CRASH(str)                                                                                                 \
    do {                                                                                                               \
        fprintf(stderr, "The GDB stub crashed: %s\n", str);                                                            \
        exit(EXIT_FAILURE);                                                                                            \
    } while (0)

// STUB
typedef enum {
    STUB_RESET = 0,
    STUB_CONNECTED = 1,
    STUB_BRKPT = 2
} Stub_State;

static const char *halted_resp[] = {
    "",
    "S05",
    "",
};

typedef struct {
    int server_fd;
    int gdb_socket;
    Stub_State state;
    // gdb client address
    struct sockaddr_in address;
    socklen_t addrlen;
    int ack_enabled;
} GDB_Stub;

// BUFFERS
typedef struct {
    unsigned char *data;
    size_t size;
    size_t filled;
    size_t start_pkt_data;
    size_t end_pkt_data;
} Buffer;

// PARSER
typedef enum {
    PARSE_RESET = 0,
    PARSE_START = 1,
    PARSE_DATA = 2,
    PARSE_END = 3,
    PARSE_ERROR = 4,
    PARSE_WRITE_RESP = 5,
    PARSE_FINISHED = 6
} Pars_State;

typedef enum {
    DATA_START,
    DATA_END,
} Pars_Data_State;

typedef enum {
    REQ_EMPTY,
    REQ_QUESTION,
    REQ_g,
    REQ_G,
    REQ_m,
    REQ_M
} Request_Type;

typedef enum {
    CHECK_DIGIT_0,
    CHECK_DIGIT_1,
} Pars_Checksum_State;

typedef struct {
    size_t pars_idx;
    Pars_State state;
    Pars_Data_State data_state;
    Pars_Checksum_State checksum_state;
    Request_Type request;
} Parser;

// STUB
static GDB_Stub server = {.state = STUB_RESET};

void gdb_stub_init(void);
void gdb_stub_handle_cmds(void);
static void gdb_stub_resp(void);
void gdb_stub_reset(void);

// PARSER
static Parser parser;

static void gdb_parser_build(int args_num, ...);
static void gdb_parser_pckt(void);
static void gdb_parser_reset(void);

// BUFFERS
static Buffer read_buffer;
static Buffer write_buffer;

static void gdb_buff_expand(Buffer *buff);
static void gdb_buff_reset(Buffer *buff);
static void gdb_buff_write(Buffer *buff, unsigned char c);

// STUB
void gdb_stub_init(void) {
    struct sockaddr_in address;

    int opt = 1;
    if ((server.server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        GDB_CRASH("Can't open TCP socket");

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(STUB_PORT);

    setsockopt(server.server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server.server_fd, (struct sockaddr *) &address, sizeof(address)) == -1)
        GDB_CRASH("Can't bind TCP socket");

    // set backlog to 1 (hint on number of outstanding connections
    // in the socket's listen queue)
    listen(server.server_fd, 1);
    printf("Waiting for GDB on port %d...\n", STUB_PORT);
    server.gdb_socket = accept(server.server_fd, (struct sockaddr *) &server.address, &server.addrlen);
    printf("GDB connected.\n");
    server.state = STUB_CONNECTED;
    server.ack_enabled = 1;
}

void gdb_stub_handle_cmds(void) {
    ssize_t rd_bytes;
    // reset everything
    gdb_buff_reset(&read_buffer);
    gdb_buff_reset(&write_buffer);
    gdb_parser_reset();

    while (1) {
        if ((read_buffer.filled + STUB_READ_SIZE) >= read_buffer.size)
            gdb_buff_expand(&read_buffer);
        rd_bytes = read(server.gdb_socket, read_buffer.data + read_buffer.filled, STUB_READ_SIZE);
        // -1 error, 0 EOF (closed connection)
        if (rd_bytes <= 0)
            GDB_CRASH("Socket read error or connection closed");
        read_buffer.filled += rd_bytes;
        // the parser will keep track of the parsed index
        // to resume the parsing were it left in the previous
        // iteration
        gdb_parser_pckt();
        if (parser.state == PARSE_FINISHED) {
            // don't reset the write in case need retransmition
            //(-)
            printf("REQEST: %s\n", read_buffer.data);
            printf("RESPONSE: %s\n", write_buffer.data);
            gdb_stub_resp();
            gdb_buff_reset(&read_buffer);
            gdb_parser_reset();
        };
    }
}

static void gdb_stub_resp(void) {
    write(server.gdb_socket, write_buffer.data, write_buffer.filled);
}

void gdb_stub_reset(void) {
    server.ack_enabled = 1;
    close(server.server_fd);
    close(server.gdb_socket);
    gdb_buff_reset(&read_buffer);
    gdb_buff_reset(&write_buffer);
    gdb_parser_reset();
}

// PARSER
static uint8_t gdb_parser_checksum(Buffer *buff) {
    size_t i = buff->start_pkt_data;
    int sum = 0;

    while (i < buff->end_pkt_data) {
        sum += (int) buff->data[i];
        i += 1;
    }
    // modulo 256
    return (uint8_t) (sum & 255);
}

static void gdb_parser_build(int args_num, ...) {
    va_list args;
    uint8_t checksum;
    char str_checksum[3];

    va_start(args, args_num);

    // start data and place start placeholder (used by checksum)
    gdb_buff_write(&write_buffer, '$');
    write_buffer.start_pkt_data = write_buffer.filled;

    for (int i = 0; i < args_num; i++) {
        const char *param = va_arg(args, const char *);
        printf("PARAM: %s\n", param);
        while (*param != '\0') {
            gdb_buff_write(&write_buffer, *param);
            param += 1;
        }
    }

    // end data and place end placeholder (used by checksum)
    write_buffer.end_pkt_data = write_buffer.filled;
    gdb_buff_write(&write_buffer, '#');

    // Place checksum
    checksum = gdb_parser_checksum(&write_buffer);
    snprintf(str_checksum, sizeof(str_checksum), "%02X", checksum);
    gdb_buff_write(&write_buffer, str_checksum[0]);
    gdb_buff_write(&write_buffer, str_checksum[1]);
    va_end(args);
}

static void parser_reset(char c) {
    if (c == '+')
        parser.state = PARSE_START;
    if (c == '-')
        // skip everything and just send the same message
        // again
        parser.state = PARSE_FINISHED;
}

static void parser_start(char c) {
    if (c == '$')
        parser.state = PARSE_DATA;
    else if (server.ack_enabled == 1)
        parser.state = PARSE_ERROR;
}

static void parser_data(char c) {
    if (parser.data_state == DATA_START) {
        // keep track of the starting of data (used by gdb_parser_checksum)
        read_buffer.start_pkt_data = parser.pars_idx;
        switch (c) {
        case '?':
            parser.request = REQ_QUESTION;
            break;
        case 'g':
            parser.request = REQ_g;
            break;
        case 'G':
            parser.request = REQ_G;
            break;
        case '#':
            parser.data_state = DATA_END;
            break;
        // if unsupported just skip to write resp, the request is already initialized to
        // REQ_EMPTY sending empty string to gdb
        default:
            parser.state = PARSE_WRITE_RESP;
            break;
        }
    }
    if (parser.data_state == DATA_END) {
        // keep track of the ending of data (used by gdb_parser_checksum)
        read_buffer.end_pkt_data = parser.pars_idx;
        parser.state = PARSE_END;
    }
}

// refactor extracting the state as another Parser variable
static void parser_end(char c) {
    static char checksum[3];
    checksum[2] = '\0';

    uint8_t value;
    switch (parser.checksum_state) {
    // passing first digit of checksum
    case CHECK_DIGIT_0:
        checksum[0] = c;
        parser.checksum_state = CHECK_DIGIT_1;
        break;
    case CHECK_DIGIT_1:
        checksum[1] = c;
        value = atoi(checksum);
        if ((server.ack_enabled == 1) && (value != gdb_parser_checksum(&read_buffer)))
            parser.state = PARSE_ERROR;
        else
            parser.state = PARSE_WRITE_RESP;
        // reset
        parser.checksum_state = CHECK_DIGIT_0;
        break;
    }
}

static void parser_error() {
    parser.state = PARSE_FINISHED;
    gdb_buff_reset(&write_buffer);
    gdb_buff_write(&write_buffer, '-');
}

static void parser_write_resp() {
    parser.state = PARSE_FINISHED;
    gdb_buff_reset(&write_buffer);
    if (server.ack_enabled == 1)
        gdb_buff_write(&write_buffer, '+');
    switch (parser.request) {
    case REQ_QUESTION:
        gdb_parser_build(1, halted_resp[server.state]);
        break;
    case REQ_g:
        gdb_parser_build(1, "g");
        break;
    case REQ_G:
        gdb_parser_build(1, "G");
        break;
    case REQ_m:
        gdb_parser_build(1, "m");
        break;
    case REQ_M:
        gdb_parser_build(1, "M");
        break;
    case REQ_EMPTY:
        gdb_parser_build(1, "");
        break;
    }
}

static void gdb_parser_pckt(void) {
    unsigned char c;
    while (parser.pars_idx < read_buffer.filled) {
        c = read_buffer.data[parser.pars_idx];
        switch (parser.state) {
        case PARSE_RESET:
            // just skip the ack
            if (server.ack_enabled != 1)
                parser.state = PARSE_START;
            else
                parser_reset(c);
            break;
        case PARSE_START:
            parser_start(c);
            break;
        case PARSE_DATA:
            parser_data(c);
            break;
        case PARSE_END:
            parser_end(c);
            break;
        case PARSE_ERROR:
            parser_error();
            break;
        case PARSE_WRITE_RESP:
            parser_write_resp();
            break;
        case PARSE_FINISHED:
            return;
            break;
        }
        parser.pars_idx += 1;
    }
}

static void gdb_parser_reset() {
    parser.state = PARSE_RESET;
    parser.data_state = DATA_START;
    parser.request = REQ_EMPTY;
    parser.checksum_state = CHECK_DIGIT_0;
    parser.pars_idx = 0;
}

// BUFFERS

static void gdb_buff_expand(Buffer *buff) {
    buff->size += STUB_READ_SIZE;
    buff->data = realloc(buff->data, buff->size);
    if (buff->data == NULL)
        GDB_CRASH("Heap memory finished (buffer realloc failed)");
}

static void gdb_buff_reset(Buffer *buff) {
    if (buff->data != NULL) {
        free(buff->data);
        buff->data = NULL;
    }
    buff->filled = 0;
    buff->size = 0;
    buff->end_pkt_data = 0;
    buff->start_pkt_data = 0;
}

// Really inefficient, should design this better
static void gdb_buff_write(Buffer *buff, unsigned char c) {
    if (buff->filled == buff->size)
        gdb_buff_expand(buff);

    buff->data[buff->filled] = c;
    buff->filled += 1;
}

void gdb_breakpoint(void) {
    // if (connected != 1)
    //
}
