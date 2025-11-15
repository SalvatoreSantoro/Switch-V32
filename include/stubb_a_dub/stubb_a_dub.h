#ifndef STUBB_A_DUB_H
#define STUBB_A_DUB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define byte unsigned char

typedef enum {
    STUB_OK,
    STUB_OOM,
    STUB_SOCKET,
    STUB_ARCH,
    STUB_SMP,
    STUB_CLOSED,
	STUB_HALTED
} stub_ret;

typedef enum {
    RV32, // the only one currently supported
} arch;

typedef enum{
	SYS_OK,
	CORE_RUNNING_ERR,
	CORE_OUT_OF_BOUNDS,
	MEMORY_OUT_OF_BOUNDS,
}sys_err;

typedef struct {
    sys_err (*read_regs)(byte *output, size_t output_sz, unsigned int core_id);
    sys_err (*write_regs)(const byte *input, size_t input_sz, unsigned int core_id);
    sys_err (*read_mem)(byte *output, size_t output_sz, uint32_t addr);
    sys_err (*write_mem)(const byte *input, size_t input_sz, uint32_t addr);
    sys_err (*core_step)(unsigned int core_id);
    void (*core_continue)(unsigned int core_id);
    void (*cores_continue)(void);
    void (*cores_halt)(void);
    bool (*is_halted)(unsigned int);
} Sys_Ops;

typedef struct {
    arch arch;
    unsigned int regs_num;
    unsigned int smp;
} Sys_Conf;

typedef struct {
    Sys_Ops sys_ops;
    Sys_Conf sys_conf;
    uint16_t port;
    size_t buffers_size;
    size_t socket_io_size;
} Stub_Conf;

stub_ret sad_stub_init(Stub_Conf *conf);

stub_ret sad_stub_handle_cmds(void);

void sad_stub_deinit(void);

#endif
