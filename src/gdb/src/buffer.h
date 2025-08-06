#ifndef BUFFER_H
#define BUFFER_H

#include "defs.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
    size_t initial_size;
    size_t filled;
    size_t data_size;
    byte *data;
} Buffer;

typedef enum {
    BUFF_OK,
    BUFF_OOM,
} buff_ret;

Buffer *sad_buff_create(size_t initial_size);

void sad_buff_destroy(Buffer *buff);

buff_ret sad_buff_init(Buffer* buff, size_t initial_size);

void sad_buff_deinit(Buffer* buff);

buff_ret sad_buff_expand(Buffer *buff);

buff_ret sad_buff_append(Buffer *buff, const byte *data, size_t data_size);

unsigned char *sad_buff_read_prep(Buffer *buff, size_t *buff_filled);

void sad_buff_print_content(Buffer *buff, const char *str);

buff_ret sad_buff_append_str(Buffer *buff, const char *str);

void sad_buff_reset(Buffer *buff);

#endif
