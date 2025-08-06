#ifndef PKT_BUFFER_H
#define PKT_BUFFER_H

#include "buffer.h"
#include <stddef.h>

typedef struct {
    Buffer buff;
    size_t start_pkt_data;
    size_t end_pkt_data;
    size_t socket_io_size;
} PKT_Buffer;

typedef enum {
    PKT_BUFF_OK,
    PKT_BUFF_OOM,
    PKT_BUFF_FD_ERR
} pkt_buff_ret;

PKT_Buffer *sad_pkt_buff_create(size_t initial_size, size_t socket_io_size);

void sad_pkt_buff_destroy(PKT_Buffer *pkt_buff);

void sad_pkt_buff_reset(PKT_Buffer *pkt_buff);

uint8_t sad_pkt_buff_checksum(PKT_Buffer *pkt_buff);

pkt_buff_ret sad_pkt_buff_from_socket(PKT_Buffer *pkt_buff, int fd);

pkt_buff_ret sad_pkt_buff_to_socket(PKT_Buffer *pkt_buff, int fd);

pkt_buff_ret sad_pkt_buff_append(PKT_Buffer *pkt_buff, const byte *data, size_t data_size);

pkt_buff_ret sad_pkt_buff_append_str(PKT_Buffer *pkt_buff, const char *data);

void sad_pkt_buff_print_content(PKT_Buffer *pkt_buff, const char *str);

#endif
