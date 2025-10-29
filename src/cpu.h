#ifndef _SV32_CPU_H
#define _SV32_CPU_H

#include <stdint.h>
#include <sys/types.h>
#include <stdbool.h>
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

// Check compress
#define COMPR_OPCODE_MASK 0x3
#define IS_COMPRESSED(x)  (((x & COMPR_OPCODE_MASK) != 0x3))

// Check opcode
#define OPCODE_MASK    0x7F
#define OPCODE_TYPE(x) (x & OPCODE_MASK)
#define R_TYPE         0x33
#define IR_TYPE        0x13
#define IL_TYPE        0x3
#define S_TYPE         0x23
#define B_TYPE         0x63
#define J_TYPE         0x6F
#define IJ_TYPE        0x67
#define LUI            0x37
#define AUIPC          0x17
#define ENV_TYPE       0x73
#define A_TYPE         0x2F

typedef struct {
    uint32_t regs[REG_NUMS];
    uint32_t pc;
    uint32_t reserved; // atomics
    // ELF
    uint32_t elf_brk;
    uint32_t elf_errno;
	// Index
	unsigned int core_idx;
} VCore;

// get register name
const char *re_na(int reg_num);

void vcore_run(VCore *core);

void vcore_step(VCore *core);

#endif
