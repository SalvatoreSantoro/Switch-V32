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
#define COMPR_OPCODE_MASK 0b11
#define IS_COMPRESSED(x)  (((x & COMPR_OPCODE_MASK) != 0b11))

// Check opcode
#define OPCODE_MASK    0b1111111
#define OPCODE_TYPE(x) (x & OPCODE_MASK)
#define R_TYPE         0b0110011
#define IR_TYPE        0b0010011
#define IL_TYPE        0b0000011
#define S_TYPE         0b0100011
#define B_TYPE         0b1100011
#define J_TYPE         0b1101111
#define IJ_TYPE        0b1100111
#define LUI            0b0110111
#define AUIPC          0b0010111
#define ENV_TYPE       0b1110011
#define A_TYPE         0b0101111

typedef struct {
    uint32_t regs[REG_NUMS];
    uint32_t pc;
    uint32_t reserved; // atomics
    // ELF
    uint32_t elf_brk;
    uint32_t elf_errno;
} VCore;

// get register name
const char *re_na(int reg_num);

void vcore_r_type(VCore *core, uint32_t ins);

void vcore_ir_type(VCore *core, uint32_t ins);

uint32_t vcore_b_type(VCore *core, uint32_t ins);

uint32_t vcore_j_type(VCore *core, uint32_t ins);

uint32_t vcore_ij_type(VCore *core, uint32_t ins);

void vcore_lui_type(VCore *core, uint32_t ins);

void vcore_auipc_type(VCore *core, uint32_t ins);

void vcore_il_type(VCore *core, uint32_t ins);

void vcore_s_type(VCore *core, uint32_t ins);

void vcore_a_type(VCore *core, uint32_t ins);

void vcore_e_type(VCore *core, uint32_t ins);

void vcore_run(VCore *core);

void vcore_step(VCore *core);

#endif
