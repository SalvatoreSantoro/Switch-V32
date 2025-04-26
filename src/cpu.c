#include "cpu.h"
#include <stdint.h>
#include <stdio.h>

// Base Integer

// Masks
#define IM_RD_MASK (0b11111 << 7)
#define RS1_MASK (0b11111 << 15)
#define RS2_MASK (0b11111 << 20)
#define IM2_F7_MASK (0b1111111 << 25)
#define F3_MASK (0b111 << 12)
#define R_IMM_MASK (0b111111111111 << 20)
#define U_IMM_MASK (0b11111111111111111111 << 12)

// Gets
#define RD(x) ((x & IM_RD_MASK) >> 7)
#define RS1(x) ((x & RS1_MASK) >> 15)
#define RS2(x) ((x & RS2_MASK) >> 20)
#define I_IMM(x) ((x & R_IMM_MASK) >> 20)
#define R_FUNC(x) (((x & F3_MASK) >> 12) | ((x & IM2_F7_MASK) >> 21))
#define FUNC(x) ((x & F3_MASK) >> 12)
#define B_IMM(x) (                   \
    (((0b1111 << 8) & x) >> 7)       \
    | (((0b111111 << 25) & x) >> 20) \
    | (((0b1 << 7) & x) << 4)        \
    | (((0b1 << 31) & x) >> 19))

#define J_IMM(x) (                     \
    (((0b1111111111 << 20) & x) >> 19) \
    | (((0b1 << 20) & x) >> 9)         \
    | (((0b11111111 << 12) & x))       \
    | (((0b1 << 31) & x) >> 11))

#define U_IMM(x) (x & (U_IMM_MASK))

// R / IR type
// func7:func3
#define ADD (0x000)
#define SUB (0x200)
#define XOR (0x004)
#define OR (0x006)
#define AND (0x007)
#define SLL (0x001)
#define SRL (0x005)
#define SRA (0x205)
#define SLT (0x002)
#define SLTU (0x003)

// B Type
// func3
#define BEQ (0x0)
#define BNE (0x1)
#define BLT (0x4)
#define BGE (0x5)
#define BLTU (0x6)
#define BGEU (0x7)

void vcore_r_type(VCore* core, uint32_t ins)
{
    uint32_t rs1 = core->regs[RS1(ins)], rs2 = core->regs[RS2(ins)];
    uint32_t* rd = &core->regs[RD(ins)];
    switch (R_FUNC(ins)) {
    case ADD:
        *rd = rs1 + rs2;
        // printf("ADD rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case SUB:
        *rd = rs1 - rs2;
        // printf("SUB rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case XOR:
        *rd = rs1 ^ rs2;
        // printf("XOR rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case OR:
        *rd = rs1 | rs2;
        // printf("OR rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case AND:
        *rd = rs1 & rs2;
        // printf("AND rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case SLL:
        *rd = rs1 << rs2;
        // printf("SLL rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case SRL:
        *rd = (unsigned)rs1 >> rs2;
        // printf("SRL rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case SRA:
        *rd = (signed)rs1 >> rs2;
        // printf("SRA rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case SLT:
        *rd = ((signed)rs1 < (signed)rs2) ? 1 : 0;
        // printf("SLT rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case SLTU:
        *rd = ((unsigned)rs1 < (unsigned)rs2) ? 1 : 0;
        // printf("SLTU rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    default:
        fprintf(stderr, "%x R-Type BADCODE\n", ins);
    }
}

void vcore_ir_type(VCore* core, uint32_t ins)
{
    uint32_t rs1 = core->regs[RS1(ins)];
    uint32_t* rd = &core->regs[RD(ins)];
    int func = R_FUNC(ins);
    int32_t imm = RS2(ins);

    if (func == SRA) {
        *rd = (signed)rs1 >> imm;
        // printf("SRA rd: %d, rs1 %d, imm %d\n", RD(ins), rs1, imm);
        return;
    }

    if (func == SRL) {
        *rd = (unsigned)rs1 >> imm;
        // printf("SRL rd: %d, rs1 %d, imm %d\n", RD(ins), rs1, imm);
        return;
    }
    if (func == SLL) {
        *rd = rs1 << imm;
        // printf("SLL rd: %d, rs1 %d, imm %d\n", RD(ins), rs1, imm);
        return;
    }

    imm = I_IMM(ins);
    func = FUNC(ins);

    switch (func) {
    case ADD:
        *rd = rs1 + imm;
        // printf("ADD rd: %d, rs1 %d, imm %d\n", RD(ins), rs1, imm);
        break;
    case XOR:
        *rd = rs1 ^ imm;
        // printf("XOR rd: %d, rs1 %d, imm %d\n", RD(ins), rs1, imm);
        break;
    case OR:
        *rd = rs1 | imm;
        // printf("OR rd: %d, rs1 %d, imm %d\n", RD(ins), rs1, imm);
        break;
    case AND:
        *rd = rs1 & imm;
        // printf("AND rd: %d, rs1 %d, imm %d\n", RD(ins), rs1, imm);
        break;
    case SLT:
        *rd = ((signed)rs1 < (signed)imm) ? 1 : 0;
        // printf("SLT rd: %d, rs1 %d, imm %d\n", RD(ins), rs1, imm);
        break;
    case SLTU:
        *rd = ((unsigned)rs1 < (unsigned)imm) ? 1 : 0;
        // printf("SLTU rd: %d, rs1 %d, imm %d\n", RD(ins), rs1, imm);
        break;
    default:
        fprintf(stderr, "%x IR-Type BADCODE\n", ins);
    }
}

void vcore_b_type(VCore* core, uint32_t ins)
{
    uint32_t rs1 = core->regs[RS1(ins)], rs2 = core->regs[RS2(ins)];
    int32_t imm = B_IMM(ins);
    switch (FUNC(ins)) {
    case BEQ:
        if (rs1 == rs2)
            core->regs[PC] += imm;
        // printf("BEQ rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case BNE:
        if (rs1 != rs2)
            core->regs[PC] += imm;
        // printf("BNE rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case BLT:
        if ((signed)rs1 < (signed)rs2)
            core->regs[PC] += imm;
        // printf("BLT rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case BGE:
        if ((signed)rs1 >= (signed)rs2)
            core->regs[PC] += imm;
        // printf("BGE rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case BLTU:
        if ((unsigned)rs1 < (unsigned)rs2)
            core->regs[PC] += imm;
        // printf("BLTU rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case BGEU:
        if ((unsigned)rs1 >= (unsigned)rs2)
            core->regs[PC] += imm;
        // printf("BGEU rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    default:
        fprintf(stderr, "%x B-Type BADCODE\n", ins);
    }
}

inline void vcore_j_type(VCore* core, uint32_t ins)
{
    core->regs[RD(ins)] = core->regs[PC] + 4;
    core->regs[PC] += J_IMM(ins);
    // printf("J rd: %d\n", RD(ins));
}

inline void vcore_ij_type(VCore* core, uint32_t ins)
{
    core->regs[RD(ins)] = core->regs[PC] + 4;
    core->regs[PC] = core->regs[RS1(ins)] + I_IMM(ins);
    // printf("IJ rd: %d\n", RD(ins));
}

inline void vcore_lui_type(VCore* core, uint32_t ins)
{
    core->regs[RD(ins)] = U_IMM(ins);
    //printf("LUI rd: %d imm: %d\n", RD(ins), U_IMM(ins));
}

inline void vcore_auipc_type(VCore* core, uint32_t ins)
{
    core->regs[RD(ins)] = core->regs[PC] + U_IMM(ins);
    //printf("AUIPC rd: %d imm: %d\n", RD(ins), U_IMM(ins));
}
