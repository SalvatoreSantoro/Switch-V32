#include "pkt_buffer.h"
#include "buffer.h"
#include <stdlib.h>
#include <unistd.h>

PKT_Buffer *sad_pkt_buff_create(size_t initial_size, size_t socket_io_size) {
    PKT_Buffer *pkt_buff;
    pkt_buff = malloc(sizeof(PKT_Buffer));
    if (pkt_buff == NULL)
        return NULL;

    if (sad_buff_init(&pkt_buff->buff, initial_size) != BUFF_OK) {
        free(pkt_buff);
        return NULL;
    }

    sad_pkt_buff_reset(pkt_buff);
    pkt_buff->socket_io_size = socket_io_size;

    return pkt_buff;
}

void sad_pkt_buff_destroy(PKT_Buffer *pkt_buff) {
    sad_buff_deinit(&pkt_buff->buff);
    free(pkt_buff);
}

void sad_pkt_buff_reset(PKT_Buffer *pkt_buff) {
    sad_buff_reset(&pkt_buff->buff);
    pkt_buff->start_pkt_data = 0;
    pkt_buff->end_pkt_data = 0;
}

pkt_buff_ret sad_pkt_buff_from_socket(PKT_Buffer *pkt_buff, int fd) {
    ssize_t rd_bytes = 0;

    while ((pkt_buff->buff.filled + pkt_buff->socket_io_size) > pkt_buff->buff.data_size) {
        if (sad_buff_expand(&pkt_buff->buff) == BUFF_OOM)
            return PKT_BUFF_OOM;
    };

    rd_bytes = read(fd, pkt_buff->buff.data + pkt_buff->buff.filled, pkt_buff->socket_io_size);
    if (rd_bytes <= 0)
        return PKT_BUFF_FD_ERR;

    pkt_buff->buff.filled += rd_bytes;
    return PKT_BUFF_OK;
}

pkt_buff_ret sad_pkt_buff_to_socket(PKT_Buffer *pkt_buff, int fd) {
    ssize_t wt_bytes = 0;
    size_t tot_wt_bytes = 0;
    size_t bytes_to_wt;
    size_t left_to_wt;

    bytes_to_wt = pkt_buff->buff.filled;
    left_to_wt = bytes_to_wt;

    while (tot_wt_bytes != bytes_to_wt) {
        wt_bytes = write(fd, pkt_buff->buff.data + tot_wt_bytes, left_to_wt);
        if (wt_bytes == -1)
            return PKT_BUFF_FD_ERR;
        left_to_wt -= wt_bytes;
        tot_wt_bytes += wt_bytes;
    }
    return PKT_BUFF_OK;
}

uint8_t sad_pkt_buff_checksum(PKT_Buffer *pkt_buff) {
    size_t i = pkt_buff->start_pkt_data;
    int sum = 0;

    while (i < pkt_buff->end_pkt_data) {
        sum += (int) pkt_buff->buff.data[i];
        i += 1;
    }
    // modulo 256
    return (uint8_t) (sum & 255);
}

pkt_buff_ret sad_pkt_buff_append(PKT_Buffer *pkt_buff, const byte *data, size_t data_size) {
    if (sad_buff_append(&pkt_buff->buff, data, data_size) != BUFF_OK)
        return PKT_BUFF_OOM;

    return PKT_BUFF_OK;
}

pkt_buff_ret sad_pkt_buff_append_str(PKT_Buffer *pkt_buff, const char *data) {
    if (sad_buff_append_str(&pkt_buff->buff, data) != BUFF_OK)
        return PKT_BUFF_OOM;

    return PKT_BUFF_OK;
}

void sad_pkt_buff_print_content(PKT_Buffer *pkt_buff, const char *str){
    sad_buff_print_content(&pkt_buff->buff, str);
}
