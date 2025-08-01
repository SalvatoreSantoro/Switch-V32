#ifndef _GDB_SUPPORTED_H
#define _GDB_SUPPORTED_H

// SAD_EXTEND START "Add new command macro"
#define SUPPORTED_CMDS                                                                                                 \
    X(q, 'q')                                                                                                          \
    X(Q, 'Q')                                                                                                          \
    X(qstmrk, '?')                                                                                                     \
    X(m, 'm')                                                                                                          \
    X(M, 'M')                                                                                                          \
    X(G, 'G')                                                                                                          \
    X(g, 'g')                                                                                                          \
    X(v, 'v')                                                                                                          \
    X(unsupported, '0') // assmuning '0' isn't a valid command
// SAD_EXTEND END

typedef enum {
#define X(s, ch) COMMAND_##s,
    SUPPORTED_CMDS
#undef X
        COMMANDS_COUNT,
} cmd_type;

extern const char supported_cmds[];

cmd_type supported_idx(const char c);

#endif
