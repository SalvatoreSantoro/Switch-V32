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

// THESE MACROS ARE NEEDED FROM BOTH CPU_USER and CPU_SUPERVISOR implementations

#define R_IMM_MASK (0xFFFu << 20)
#define F3_MASK    (0x7u << 12)
#define IM_RD_MASK (0x1Fu << 7)
#define RS1_MASK   (0x1Fu << 15)

#define RD(x)   (uint32_t) ((x & IM_RD_MASK) >> 7)
#define RS1(x)  (uint32_t) ((x & RS1_MASK) >> 15)
#define FUNC(x) (uint32_t) ((x & F3_MASK) >> 12)
// Immediates are always int32_t (sign-extended) before shifting
#define I_IMM(x) (uint32_t) ((int32_t) (x & R_IMM_MASK) >> 20)

// SYS Type
#define ECALL  (0x0)
#define EBREAK (0x1)

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
    uint32_t scause;
    uint32_t stval;
    uint32_t stvec;
    uint32_t sepc;
    uint32_t sscratch;
#elif USER
    // ELF
    uint32_t elf_brk;
    uint32_t elf_errno;
#endif
} VCore;

// get register name
const char *re_na(int reg_num);

void vcore_run(VCore *core);

void vcore_step(VCore *core);

// different implementations for USER and SUPERVISOR
void vcore_sys_type(VCore *core, uint32_t ins);

#endif
