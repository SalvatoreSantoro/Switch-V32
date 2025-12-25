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
    STUB_HALTED,
    STUB_UNEXPECTED
} stub_ret;

typedef enum {
    RV32, // the only one currently supported
} arch;

// when writing and reading register we're assuming that the registers width
// matches the configured "arch" specified
// so 4 or 8 bytes depending on the XLEN
typedef struct {
    const char *(*core_name)(uint32_t core_id);
    uint64_t (*read_reg)(uint32_t core_id, uint32_t reg_id);
    void (*write_reg)(uint64_t input, uint32_t core_id, uint32_t reg_id);
    void (*read_mem)(byte *output, uint64_t output_sz, uint64_t addr);
    void (*write_mem)(const byte *input, uint64_t input_sz, uint64_t addr);
    void (*core_step)(unsigned int core_id);
    void (*core_continue)(unsigned int core_id);
    void (*cores_continue)(void);
    void (*cores_halt)(void);
    bool (*is_halted)(void);
} Sys_Ops;

typedef struct {
    arch arch;
    uint32_t pc_id;
    uint32_t regs_num;
    uint32_t smp;
    uint64_t memory_size;
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
