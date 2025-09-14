#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    size_t initial_size;
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

PKT_Buffer *sad_buff_create(size_t initial_size, size_t socket_io_size);

void sad_buff_destroy(PKT_Buffer *buff);

// returns -1 on error, 0 otherwise
buff_ret sad_buff_expand(PKT_Buffer *buff);

// returns -1 on error, 0 otherwise
buff_ret sad_buff_append(PKT_Buffer *buff, const char *data, size_t data_size);

unsigned char *sad_buff_read_prep(PKT_Buffer *buff, size_t *buff_filled);

buff_ret sad_buff_from_socket(PKT_Buffer *buff, int fd);

buff_ret sad_buff_to_socket(PKT_Buffer *buff, int fd);

void sad_buff_print_content(PKT_Buffer *buff, const char *str);

buff_ret sad_buff_append_str(PKT_Buffer *buff, const char *str);

uint8_t sad_buff_checksum(PKT_Buffer *buff);

void sad_buff_reset(PKT_Buffer *buff);

#endif
