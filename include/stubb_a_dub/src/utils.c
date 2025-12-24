#include "sad_gdb_internal.h"
#include "stubb_a_dub.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

util_ret sad_hex_chars_to_bytes(byte *dest, const char *src, size_t dest_size, size_t src_size) {
    if (dest_size < ((src_size + 1) / 2)) {
        return UTIL_UNEXPECTED;
    }

    // 0 first hex symbol
    // 1 second hex symbol
    // based on the length of the src, the parsing changes
    bool state = (src_size % 2) == 0 ? 1 : 0;

    char c;
    byte tmp_val = 0;
    size_t bytes_idx = 0;

    for (size_t i = 0; i < src_size; i++) {
        c = src[i];
        if (c >= '0' && c <= '9')
            c -= '0';
        else if (c >= 'A' && c <= 'F')
            c = c - 'A' + 10;
        else if (c >= 'a' && c <= 'f')
            c = c - 'a' + 10;
        else
            return UTIL_X;

        if (state) {
            c <<= 4;
            tmp_val = (byte) c;
        } else {
            tmp_val += (byte) c;
            *(dest + bytes_idx) = tmp_val;
            bytes_idx += 1;
        }

        state = !state;
    }

    // reset the remaining bytes
    while (bytes_idx != dest_size) {
        *(dest + bytes_idx) = 0;
        bytes_idx += 1;
    }
    return UTIL_OK;
}

util_ret sad_hex_str_to_bytes(byte *dest, const char *src, size_t dest_size) {
    size_t src_size = strlen(src);
    return sad_hex_chars_to_bytes(dest, src, dest_size, src_size);
}

util_ret sad_bytes_to_hex_chars(char *dest, const byte *src, size_t dest_size, size_t src_size) {
    const char hex[] = "0123456789abcdef";
    byte v;

    if (dest_size < (2 * src_size))
        return UTIL_UNEXPECTED;

    // 2 chars for every byte
    for (size_t i = 0; i < src_size; i++) {
        v = src[i];
        dest[2 * i] = hex[v >> 4];
        dest[2 * i + 1] = hex[v & 0xF];
    }

    return UTIL_OK;
}
