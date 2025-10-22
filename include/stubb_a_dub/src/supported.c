#include "supported.h"

const char supported_cmds[] = {
#define X(s, ch) ch,
    SUPPORTED_CMDS
#undef X
    '\0'};

cmd_type supported_idx(const char c) {
    int idx = 0;
    while (*(supported_cmds + idx) != '\0') {
        if (supported_cmds[idx] == c)
            return idx;
        idx += 1;
    }
    return COMMAND_unsupported;
}
