#include "gdb.h"
#include "defs.h"
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// The stub ignores the checksum cause the communication is on TCP, so
// the checks are redundant

#define GDB_CRASH(str)                                                                                                 \
    do {                                                                                                               \
        fprintf(stderr, "The GDB stub crashed: %s\n", str);                                                            \
        exit(EXIT_FAILURE);                                                                                            \
    } while (0)

// Connection states
typedef enum {
    STUB_RESET,
    STUB_PCKT_RCVD
}STUB_State;

typedef enum{
    PROC_RESET,
    PROC_START_OK,
    PROC_END_OK
}Processing_State;

typedef struct {
    unsigned char *data;
    size_t size;
    size_t filled;
} Buffer;

typedef struct {
    Buffer buff;
    unsigned int packet_data_start;
    unsigned int packet_data_end;
    Processing_State state;
} STUB_Buffer;

static STUB_Buffer read_buffer;
static Buffer write_buffer;

static int server_fd;
static int gdb_socket;
static int connected = 0;
static STUB_State state = STUB_RESET;

// static unsigned char* packet_checksum;

void gdb_stub_init(void) {
    // gdb client address
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    int opt = 1;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        GDB_CRASH("Can't open TCP socket");

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(STUB_PORT);

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) == -1)
        GDB_CRASH("Can't bind TCP socket");

    // set backlog to 1 (hint on number of outstanding connections
    // in the socket's listen queue)
    listen(server_fd, 1);
    printf("Waiting for GDB on port %d...\n", STUB_PORT);
    gdb_socket = accept(server_fd, (struct sockaddr *) &address, &addrlen);
    printf("GDB connected.\n");
    connected = 1;
}

static void gdb_expand_buff(Buffer *buff) {
    buff->size += STUB_READ_SIZE;
    buff->data = realloc(buff->data, buff->size);
    if (buff->data == NULL)
        GDB_CRASH("Heap memory finished (buffer realloc failed)");
}

static void gdb_process_pckt() {
    unsigned int tmp_idx;
    switch(read_buffer.state){
        case PROC_RESET:
            tmp_idx = read_buffer.packet_data_start;
            break;
        case PROC_START_OK:
            tmp_idx = read_buffer.packet_data_end;
            break;
        case PROC_END_OK:
            break;
    };

    while ((read_buffer.packet_data_end != 0) && ()) {

        idx += 1;
    }

}

static void gdb_reset_read_buff(void) {
    read_buffer.buff.filled = 0;
    read_buffer.packet_data_start = 0;
    read_buffer.packet_data_end = 0;
    read_buffer.state = PROC_RESET;
}

void gdb_handle_cmds(void) {
    ssize_t rd_bytes;
    // reset the buffer
    gdb_reset_read_buff();

    while (1) {
        if ((read_buffer.buff.filled + STUB_READ_SIZE) >= read_buffer.buff.size)
            gdb_expand_buff(&read_buffer.buff);
        rd_bytes = read(gdb_socket, read_buffer.buff.data + read_buffer.buff.filled, STUB_READ_SIZE);
        // -1 error, 0 EOF (closed connection)
        if (rd_bytes <= 0)
            GDB_CRASH("Socket read error or connection closed");
        read_buffer.buff.filled += rd_bytes;
        // finished parsing the request, respond and reset buffer
        // in order to accomodate a new request
        gdb_process_pckt();
        if (state == STUB_PCKT_RCVD) {
            // RESPOND
            gdb_reset_read_buff();
            state = STUB_RESET;
        }
    }
}

void gdb_breakpoint(void) {
    // if (connected != 1)
    //
}

void gdb_stub_deinit(void) {
    close(server_fd);
    close(gdb_socket);
    if (read_buffer.buff.data != NULL)
        free(read_buffer.buff.data);
    if (write_buffer.data != NULL)
        free(write_buffer.data);
}
