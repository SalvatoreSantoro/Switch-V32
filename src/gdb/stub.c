#include "gdb.h"
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


static GDB_Stub server = {.state = STUB_RESET};


// STUB
void gdb_stub_init(int port) {
    struct sockaddr_in address;

    int opt = 1;
    if ((server.server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        GDB_CRASH("Can't open TCP socket");

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    setsockopt(server.server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server.server_fd, (struct sockaddr *) &address, sizeof(address)) == -1)
        GDB_CRASH("Can't bind TCP socket");

    // set backlog to 1 (hint on number of outstanding connections
    // in the socket's listen queue)
    listen(server.server_fd, 1);
    printf("Waiting for GDB on port %d...\n", port);
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
