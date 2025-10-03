#ifndef STUB_H
#define STUB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define byte unsigned char

typedef enum {
    STUB_OK,
    STUB_OOM,
    STUB_SOCKET,
    STUB_ARCH,
    STUB_SMP
} stub_ret;

typedef enum {
    RV32, // the only one currently supported
} arch;

typedef struct {
    void (*read_regs)(byte *output, size_t output_sz, int core_id);
    void (*write_regs)(const byte *input, size_t input_sz, int core_id);
    void (*read_mem)(byte *output, size_t output_sz, uint32_t addr);
    void (*write_mem)(const byte *input, size_t input_sz, uint32_t addr);
    void (*core_step)(int core_id);
    void (*core_continue)(int core_id);
    void (*cores_continue)();
    void (*cores_halt)();
    bool (*is_halted)(int);
} Sys_Ops;

typedef struct {
    arch arch;
    int regs_num;
    int smp;
} Sys_Conf;

typedef struct {
    Sys_Ops sys_ops;
    Sys_Conf sys_conf;
    int port;
    size_t buffers_size;
    size_t socket_io_size;
} Stub_Conf;

stub_ret sad_stub_init(Stub_Conf conf);

stub_ret sad_stub_handle_cmds(void);

void sad_stub_reset(void);

#endif
