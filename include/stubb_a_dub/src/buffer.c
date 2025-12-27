#include "defs.h"
#include "sad_gdb_internal.h"
#include <errno.h> // IWYU pragma: export
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

PKT_Buffer *sad_buff_create(size_t initial_size) {
    PKT_Buffer *buff;
    unsigned char *data;

    if (initial_size == 0) 
        return NULL;

    buff = malloc(sizeof(PKT_Buffer));
    if (buff == NULL)
        return NULL;

    data = malloc(initial_size);
    if (data == NULL) {
        free(buff);
        return NULL;
    }

    buff->initial_size = initial_size;
    buff->data_size = initial_size;
    buff->data = data;
    sad_buff_reset(buff);
    return buff;
}

void sad_buff_destroy(PKT_Buffer *buff) {
    free(buff->data);
    free(buff);
}

void sad_buff_reset(PKT_Buffer *buff) {
    if (buff->data_size >= MAX_BUFF_DATA_SIZE) {
        free(buff->data);
        buff->data = malloc(buff->initial_size);
    }
    buff->filled = 0;
    buff->end_pkt_data = 0;
    buff->start_pkt_data = 0;
}

buff_ret sad_buff_expand(PKT_Buffer *buff) {
    unsigned char *tmp_data;
    tmp_data = realloc(buff->data, buff->data_size * 2);
    if (tmp_data == NULL)
        return BUFF_OOM;
    buff->data_size *= 2;
    buff->data = tmp_data;
    return BUFF_OK;
}

buff_ret sad_buff_append_str(PKT_Buffer *buff, const char *str) {
    return sad_buff_append(buff, str, strlen(str));
}

buff_ret sad_buff_append(PKT_Buffer *buff, const char *data, size_t data_size) {
    while ((buff->filled + data_size) > buff->data_size) {
        if (sad_buff_expand(buff) == BUFF_OOM)
            return BUFF_OOM;
    }
    memcpy(buff->data + buff->filled, data, data_size);
    buff->filled += data_size;

    return BUFF_OK;
}

byte *sad_buff_read_prep(const PKT_Buffer *buff, size_t *buff_filled) {
    if (buff_filled != NULL)
        *buff_filled = buff->filled;
    return buff->data;
}

// gcc gives warnings on if (errno == EAGAIN || errno == EWOULDBLOCK)

#if defined(__GNUC__) && !defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wlogical-op"
#endif

buff_ret sad_buff_from_socket(PKT_Buffer *buff, int fd) {
    ssize_t rd_bytes = 0;

    while ((buff->filled + buff->initial_size) > buff->data_size) {
        if (sad_buff_expand(buff) == BUFF_OOM)
            return BUFF_OOM;
    };

    do {
        rd_bytes = read(fd, buff->data + buff->filled, buff->initial_size);
    } while (rd_bytes == -1 && errno == EINTR);

    if (rd_bytes == -1) {

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return BUFF_WOULD_BLOCK;
        } else {
            return BUFF_FD_ERR;
        }
    } else if (rd_bytes == 0) {
        return BUFF_CLOSED; // EOF
    }

    buff->filled += (size_t) rd_bytes;
    return BUFF_OK;
}

buff_ret sad_buff_to_socket(PKT_Buffer *buff, int fd) {
    ssize_t wt_bytes = 0;
    size_t tot_wt_bytes = 0;
    size_t bytes_to_wt;
    size_t left_to_wt;

    bytes_to_wt = buff->filled;
    left_to_wt = bytes_to_wt;

    while (tot_wt_bytes != bytes_to_wt) {
        do {
            wt_bytes = write(fd, buff->data + tot_wt_bytes, left_to_wt);
        } while (wt_bytes == -1 && errno == EINTR);

        if (wt_bytes == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return BUFF_WOULD_BLOCK;
            return BUFF_FD_ERR;
        }

        if (wt_bytes == 0) {
            return BUFF_FD_ERR; // should not happen unless weird fd
        }

        left_to_wt -= (size_t) wt_bytes;
        tot_wt_bytes += (size_t) wt_bytes;
    }
    return BUFF_OK;
}
#if defined(__GNUC__) && !defined(__clang__)
    #pragma GCC diagnostic pop
#endif

void sad_buff_print_content(const PKT_Buffer *buff, const char *str) {
    printf("%s", str);
    for (size_t i = 0; i < buff->filled; i++)
        putchar(buff->data[i]);
    putchar('\n');
}

uint8_t sad_buff_checksum(PKT_Buffer *buff) {
    size_t i = buff->start_pkt_data;
    int sum = 0;

    while (i < buff->end_pkt_data) {
        sum += (int) buff->data[i];
        i += 1;
    }
    // modulo 256
    return (uint8_t) (sum & 255);
}
