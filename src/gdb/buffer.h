#ifndef _GDB_BUFF_H
#define _GDB_BUFF_H

#include <stddef.h>
#include <stdint.h>
#include "data.h"

typedef struct {
    size_t start_pkt_data;
    size_t end_pkt_data;
    size_t socket_io_size;
    size_t filled;
    size_t data_size;
    unsigned char *data;
} PKT_Buffer;

typedef enum {
    BUFF_OK,
    BUFF_OOM,
    BUFF_FD_ERR
} buff_ret;

PKT_Buffer *gdb_buff_create(size_t initial_size, size_t socket_io_size);

void gdb_buff_destroy(PKT_Buffer *buff);

// returns -1 on error, 0 otherwise
buff_ret gdb_buff_expand(PKT_Buffer *buff);

// returns -1 on error, 0 otherwise
buff_ret gdb_buff_append(PKT_Buffer *buff, const unsigned char *data, size_t data_size);

unsigned char *gdb_buff_read_prep(PKT_Buffer *buff, size_t *buff_filled);

buff_ret gdb_buff_from_socket(PKT_Buffer *buff, int fd);

buff_ret gdb_buff_to_socket(PKT_Buffer *buff, int fd);

uint8_t gdb_buff_checksum(PKT_Buffer *buff);

#define gdb_buff_reset(buff)                                                                                           \
    do {                                                                                                               \
        buff->filled = 0;                                                                                              \
        buff->end_pkt_data = 0;                                                                                        \
        buff->start_pkt_data = 0;                                                                                      \
    } while (0)

#define gdb_buff_get_start_pkt_data(buff) (buff->start_pkt_data)

#define gdb_buff_set_start_pkt_data(buff, idx) (buff->start_pkt_data = idx)

#define gdb_buff_get_end_pkt_data(buff) (buff->end_pkt_data)

#define gdb_buff_set_end_pkt_data(buff, idx) (buff->end_pkt_data = idx)

#endif
