#ifndef SV32_CPU_H
#define SV32_CPU_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
// Register list using X-Macro pattern
#define REG_LIST                                                                                                       \
    X(ZERO, 0) /* x0 */                                                                                                \
    X(RA, 1)   /* x1 */                                                                                                \
    X(SP, 2)   /* x2 */                                                                                                \
    X(GP, 3)   /* x3 */                                                                                                \
    X(TP, 4)   /* x4 */                                                                                                \
    X(T0, 5)   /* x5 */                                                                                                \
    X(T1, 6)   /* x6 */                                                                                                \
    X(T2, 7)   /* x7 */                                                                                                \
    X(S0, 8)   /* x8 (FP) */                                                                                           \
    X(S1, 9)   /* x9 */                                                                                                \
    X(A0, 10)  /* x10 */                                                                                               \
    X(A1, 11)  /* x11 */                                                                                               \
    X(A2, 12)  /* x12 */                                                                                               \
    X(A3, 13)  /* x13 */                                                                                               \
    X(A4, 14)  /* x14 */                                                                                               \
    X(A5, 15)  /* x15 */                                                                                               \
    X(A6, 16)  /* x16 */                                                                                               \
    X(A7, 17)  /* x17 */                                                                                               \
    X(S2, 18)  /* x18 */                                                                                               \
    X(S3, 19)  /* x19 */                                                                                               \
    X(S4, 20)  /* x20 */                                                                                               \
    X(S5, 21)  /* x21 */                                                                                               \
    X(S6, 22)  /* x22 */                                                                                               \
    X(S7, 23)  /* x23 */                                                                                               \
    X(S8, 24)  /* x24 */                                                                                               \
    X(S9, 25)  /* x25 */                                                                                               \
    X(S10, 26) /* x26 */                                                                                               \
    X(S11, 27) /* x27 */                                                                                               \
    X(T3, 28)  /* x28 */                                                                                               \
    X(T4, 29)  /* x29 */                                                                                               \
    X(T5, 30)  /* x30 */                                                                                               \
    X(T6, 31)  /* x31 */

enum {
#define X(name, value) name = value,
    REG_LIST
#undef X
        REG_NUMS
};

#ifdef SUPERVISOR
typedef enum {
    // supervisor is 0 so when resetting the
    // memory area of the core we ensure to start in supervisor as default
    SUPERVISOR_MODE = 0,
    USER_MODE = 1
} execution_mode;
#endif

typedef struct {
    uint32_t regs[REG_NUMS];
    uint32_t pc;
    uint32_t reserved; // atomics
    // Index
    unsigned int core_idx;
#ifdef SUPERVISOR
    execution_mode mode;
    // CSRs
    uint32_t satp;
    uint32_t sstatus;
#else
    // ELF
    uint32_t elf_brk;
    uint32_t elf_errno;
#endif
} VCore;

// get register name
const char *re_na(int reg_num);

void vcore_run(VCore *core);

void vcore_step(VCore *core);

#endif
