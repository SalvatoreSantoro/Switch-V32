#include "sad_gdb_internal.h"
#include "stubb_a_dub.h"
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

extern PKT_Buffer *input_buffer_g;

static char *sad_cursor_tokenizer(size_t *cursor, int term) {
    // crash if calling this AFTER reaching the EOB (SEGFAULT)
    assert(*(cursor) -1 != input_buffer_g->end_pkt_data);

    char *start = (char *) (input_buffer_g->data + *cursor);
    size_t len = input_buffer_g->end_pkt_data - *cursor;
    size_t idx = 0;

    while (idx != len) {
        idx++;

        // early exit on terminator only if different from EOF
        if (term != EOB && (*(start + idx) == term)) {
            break;
        }
    }

    *(start + idx) = '\0';

    *cursor += ++idx;
    return start;
}


stub_ret sad_extract_uint32(uint32_t *val, size_t *cursor, int term) {
    char *uint32_str;

    uint32_str = sad_cursor_tokenizer(cursor, term);

    long uint32 = strtol(uint32_str, NULL, 16);

    if (uint32 > UINT32_MAX || uint32 < 0)
        return STUB_UNEXPECTED;

    *val = (uint32_t) uint32;

    return STUB_OK;
}


stub_ret sad_extract_uint64(uint64_t *val, size_t *cursor, int term) {
    char *uint64_str;

    uint64_str = sad_cursor_tokenizer(cursor, term);

    long uint64 = strtol(uint64_str, NULL, 16);

    if (uint64 == LONG_MAX || uint64 < 0)
        return STUB_UNEXPECTED;

    *val = (size_t) uint64;

    return STUB_OK;
}

char *sad_extract_str(size_t *cursor, int term) {
    return sad_cursor_tokenizer(cursor, term);
}
