#ifndef SUPPORTED_H
#define SUPPORTED_H

// SAD_EXTEND START "Add new command macro"
#define SUPPORTED_CMDS                                                                                                 \
    X(p, 'p')                                                                                                          \
    X(P, 'P')                                                                                                          \
    X(q, 'q')                                                                                                          \
    X(Q, 'Q')                                                                                                          \
    X(qstmrk, '?')                                                                                                     \
    X(m, 'm')                                                                                                          \
    X(M, 'M')                                                                                                          \
    X(G, 'G')                                                                                                          \
    X(g, 'g')                                                                                                          \
    X(T, 'T')                                                                                                          \
    X(z, 'z')                                                                                                          \
    X(Z, 'Z')                                                                                                          \
    X(H, 'H')                                                                                                          \
    X(v, 'v')                                                                                                          \
    X(unsupported, '0') // assuming '0' isn't a valid command
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
