#ifndef UTILS_H
#define UTILS_H

#include "stub.h"
#include <stddef.h>

typedef enum {
    UTIL_SIZE_ERROR,
    UTIL_NO_HEX,
    UTIL_OK
} util_ret;

util_ret sad_hex_chars_to_bytes(byte *dest, const char *src, size_t dest_size, size_t src_size);

util_ret sad_hex_str_to_bytes(byte *dest, const char *src, size_t dest_size);

util_ret sad_bytes_to_hex_chars(char *dest, const byte *src, size_t dest_size, size_t src_size);

#endif
