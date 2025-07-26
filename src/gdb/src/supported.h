#ifndef _GDB_SUPPORTED_H
#define _GDB_SUPPORTED_H

// SAD_EXTEND START "Add new command macro"
#define SUPPORTED_CMDS \
    X(q, 'q') \
    X(Q, 'Q') \
    X(QSTMRK, '?') \
    X(m, 'm') \
    X(M, 'M')
// SAD_EXTEND END

typedef enum {
    #define X(s, ch) COMMAND_##s,
    SUPPORTED_CMDS
    #undef X
    COMMANDS_COUNT,
    COMMAND_UNSUPPORTED
} CMD_Type;

const char supported_cmds[] = {
    #define X(s, ch) ch,
    SUPPORTED_CMDS
    #undef X
    '\0'
};

static CMD_Type supported_idx(const char c) {
    int idx = 0;
    while (*(supported_cmds + idx) != '\0') {
        if (supported_cmds[idx] == c)
            return idx;
        idx += 1;
    }
    return -1;
}


#endif
