#ifndef _GDB_STUB_H
#define _GDB_STUB_H

#include <stddef.h>
typedef enum {
    STUB_OK,
    STUB_OOM,
    STUB_SOCKET
} stub_ret;


stub_ret gdb_stub_init(int port, size_t buffers_size, size_t socket_io_size);

stub_ret gdb_stub_handle_cmds(void);

void gdb_stub_reset(void);

#endif
