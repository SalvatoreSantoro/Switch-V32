#include "buffer.h"
#include "defs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

Buffer *sad_buff_create(size_t initial_size) {
    Buffer *buff;

    if (initial_size == 0)
        return NULL;

    buff = malloc(sizeof(Buffer));
    if (buff == NULL)
        return NULL;

    if (sad_buff_init(buff, initial_size) != BUFF_OK)
        return NULL;

    return buff;
};

buff_ret sad_buff_init(Buffer *buff, size_t initial_size) {
    byte *data;

    data = malloc(initial_size);

    if (data == NULL) {
        free(buff);
        return BUFF_OOM;
    }

    buff->initial_size = initial_size;
    buff->data_size = initial_size;
    buff->data = data;
    sad_buff_reset(buff);

    return BUFF_OK;
}

void sad_buff_deinit(Buffer *buff) {
    buff->initial_size = 0;
    buff->data_size = 0;
    free(buff->data);
    buff->data = NULL;
}

void sad_buff_destroy(Buffer *buff) {
    free(buff->data);
    free(buff);
    buff = NULL;
};

void sad_buff_reset(Buffer *buff) {
    if (buff->data_size >= MAX_BUFF_DATA_SIZE) {
        free(buff->data);
        buff->data = malloc(buff->initial_size);
    }
    buff->filled = 0;
}

buff_ret sad_buff_expand(Buffer *buff) {
    unsigned char *tmp_data;
    tmp_data = realloc(buff->data, buff->data_size * 2);
    if (tmp_data == NULL)
        return BUFF_OOM;
    buff->data_size *= 2;
    buff->data = tmp_data;
    return BUFF_OK;
}

buff_ret sad_buff_append_str(Buffer *buff, const char *str) {
    return sad_buff_append(buff, (byte *) str, strlen(str));
}

buff_ret sad_buff_append(Buffer *buff, const byte *data, size_t data_size) {
    while ((buff->filled + data_size) > buff->data_size) {
        if (sad_buff_expand(buff) == BUFF_OOM)
            return BUFF_OOM;
    }
    memcpy(buff->data + buff->filled, data, data_size);
    buff->filled += data_size;

    return BUFF_OK;
}

unsigned char *sad_buff_read_prep(Buffer *buff, size_t *buff_filled) {
    if (buff_filled != NULL)
        *buff_filled = buff->filled;
    return buff->data;
}

void sad_buff_print_content(Buffer *buff, const char *str) {
    printf("%s", str);
    for (size_t i = 0; i < buff->filled; i++)
        putchar(buff->data[i]);
    putchar('\n');
}
