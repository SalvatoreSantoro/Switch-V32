#ifndef STUB_H
#define STUB_H

#include "builder.h"
#include "buffer.h"
#include "callback.h"
#include <stddef.h>

typedef enum {
    STUB_OK,
    STUB_OOM,
    STUB_SOCKET
} stub_ret;

stub_ret sad_stub_init(int port, size_t buffers_size, size_t socket_io_size);

stub_ret sad_stub_handle_cmds(void);

void sad_stub_reset(void);

#endif
