#include "cpu.h"
#include "memory.h"
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
#define S_IMM(x) ((RD(x)) | ((x & IM2_F7_MASK) >> 20)) // 19?

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

// IL Type
// func3
#define LB (0x0)
#define LH (0x1)
#define LW (0x2)
#define LBU (0x4)
#define LHU (0x5)

// S Type
// func3
#define SB (0x0)
#define SH (0x1)
#define SW (0x2)

const char* re_na(int reg_num)
{
    switch (reg_num) {
#define X(name, value) \
    case value:        \
        return #name;
        REG_LIST
#undef X
    default:
        return "UNKNOWN_REG";
    }
}

void vcore_r_type(VCore* core, uint32_t ins)
{
    uint32_t rs1 = core->regs[RS1(ins)], rs2 = core->regs[RS2(ins)];
    uint32_t* rd = &core->regs[RD(ins)];
    switch (R_FUNC(ins)) {
    case ADD:
        *rd = rs1 + rs2;
        printf("ADD ");
        break;
    case SUB:
        *rd = rs1 - rs2;
        printf("SUB ");
        break;
    case XOR:
        *rd = rs1 ^ rs2;
        printf("XOR ");
        break;
    case OR:
        *rd = rs1 | rs2;
        printf("OR ");
        break;
    case AND:
        *rd = rs1 & rs2;
        printf("AND ");
        break;
    case SLL:
        *rd = rs1 << rs2;
        printf("SLL ");
        break;
    case SRL:
        *rd = (unsigned)rs1 >> rs2;
        printf("SRL ");
        break;
    case SRA:
        *rd = (signed)rs1 >> rs2;
        printf("SRA ");
        break;
    case SLT:
        *rd = ((signed)rs1 < (signed)rs2) ? 1 : 0;
        printf("SLT ");
        break;
    case SLTU:
        *rd = ((unsigned)rs1 < (unsigned)rs2) ? 1 : 0;
        printf("SLTU ");
        break;
    default:
        fprintf(stderr, "%x R-Type BADCODE\n", ins);
    }
    printf("rd: %s, rs1: %s, rs2: %s\n", re_na(RD(ins)), re_na(RS1(ins)), re_na(RS2(ins)));
    printf("content now:  %x, %x, %x\n", core->regs[RD(ins)], core->regs[RS1(ins)], core->regs[RS2(ins)]);
}

void vcore_ir_type(VCore* core, uint32_t ins)
{
    uint32_t rs1 = core->regs[RS1(ins)];
    uint32_t* rd = &core->regs[RD(ins)];
    int func = R_FUNC(ins);
    int32_t imm = RS2(ins);

    if (func == SRA) {
        *rd = (signed)rs1 >> imm;
        printf("SRA ");
        printf("rd: %s, rs1: %s, imm: %x\n", re_na(RD(ins)), re_na(RS1(ins)), imm);
        printf("content now:  %x, %x\n", core->regs[RD(ins)], core->regs[RS1(ins)]);
        return;
    }

    if (func == SRL) {
        *rd = (unsigned)rs1 >> imm;
        printf("SRL ");
        printf("rd: %s, rs1: %s, imm: %x\n", re_na(RD(ins)), re_na(RS1(ins)), imm);
        printf("content now:  %x, %x\n", core->regs[RD(ins)], core->regs[RS1(ins)]);
        return;
    }
    if (func == SLL) {
        *rd = rs1 << imm;
        printf("SLL ");
        printf("rd: %s, rs1: %s, imm: %x\n", re_na(RD(ins)), re_na(RS1(ins)), imm);
        printf("content now:  %x, %x\n", core->regs[RD(ins)], core->regs[RS1(ins)]);
        return;
    }

    imm = I_IMM(ins);
    func = FUNC(ins);

    switch (func) {
    case ADD:
        *rd = rs1 + imm;
        printf("ADD ");
        break;
    case XOR:
        *rd = rs1 ^ imm;
        printf("XOR ");
        break;
    case OR:
        *rd = rs1 | imm;
        printf("OR ");
        break;
    case AND:
        *rd = rs1 & imm;
        printf("AND ");
        break;
    case SLT:
        *rd = ((signed)rs1 < (signed)imm) ? 1 : 0;
        printf("SLT ");
        break;
    case SLTU:
        *rd = ((unsigned)rs1 < (unsigned)imm) ? 1 : 0;
        printf("SLTU ");
        break;
    default:
        fprintf(stderr, "%x IR-Type BADCODE\n", ins);
    }
    printf("rd: %s, rs1: %s, imm: %x\n", re_na(RD(ins)), re_na(RS1(ins)), imm);
    printf("content now:  %x, %x\n", core->regs[RD(ins)], core->regs[RS1(ins)]);
}

void vcore_b_type(VCore* core, uint32_t ins)
{
    uint32_t rs1 = core->regs[RS1(ins)], rs2 = core->regs[RS2(ins)];
    int32_t imm = B_IMM(ins);
    switch (FUNC(ins)) {
    case BEQ:
        if (rs1 == rs2)
            core->regs[PC] += imm;
        printf("BEQ rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case BNE:
        if (rs1 != rs2)
            core->regs[PC] += imm;
        printf("BNE rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case BLT:
        if ((signed)rs1 < (signed)rs2)
            core->regs[PC] += imm;
        printf("BLT rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case BGE:
        if ((signed)rs1 >= (signed)rs2)
            core->regs[PC] += imm;
        printf("BGE rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case BLTU:
        if ((unsigned)rs1 < (unsigned)rs2)
            core->regs[PC] += imm;
        printf("BLTU rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case BGEU:
        if ((unsigned)rs1 >= (unsigned)rs2)
            core->regs[PC] += imm;
        printf("BGEU rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    default:
        fprintf(stderr, "%x B-Type BADCODE\n", ins);
    }
    printf("rs1: %s, rs2: %s, imm: %x\n", re_na(RD(ins)), re_na(RS1(ins)), imm);
    printf("content now: %x, %x\n", core->regs[RS1(ins)], core->regs[RS2(ins)]);
}

inline void vcore_j_type(VCore* core, uint32_t ins)
{
    core->regs[RD(ins)] = core->regs[PC] + 4;
    core->regs[PC] += J_IMM(ins);
    printf("J rd: %s, imm: %x\n", re_na(RD(ins)), J_IMM(ins));
}

inline void vcore_ij_type(VCore* core, uint32_t ins)
{
    core->regs[RD(ins)] = core->regs[PC] + 4;
    core->regs[PC] = core->regs[RS1(ins)] + I_IMM(ins);
    printf("IJ rd: %s, rs1: %s , imm: %x\n", re_na(RD(ins)), re_na(RS1(ins)), I_IMM(ins));
}

inline void vcore_lui_type(VCore* core, uint32_t ins)
{
    core->regs[RD(ins)] = U_IMM(ins);
    printf("LUI rd: %s, imm: %x\n", re_na(RD(ins)), J_IMM(ins));
}

inline void vcore_auipc_type(VCore* core, uint32_t ins)
{
    core->regs[RD(ins)] = core->regs[PC] + U_IMM(ins);
    printf("AUIPC rd: %s, imm: %x\n", re_na(RD(ins)), J_IMM(ins));
    printf("content now: %x\n", core->regs[RD(ins)]);
}

void vcore_il_type(VCore* core, uint32_t ins)
{
    uint32_t rs1 = core->regs[RS1(ins)];
    uint32_t* rd = &core->regs[RD(ins)];
    int32_t imm = I_IMM(ins);
    // LB and LH need sign extend
    switch (FUNC(ins)) {
    case LB:
        *rd = mem_rb(rs1 + imm);
        if (*rd >= 0b10000000)
            *rd |= 0xFFFFFF00;
        printf("LB ");
        break;
    case LH:
        *rd = mem_rh(rs1 + imm);
        if (*rd >= 0b1000000000000000)
            *rd |= 0xFFFF0000;
        printf("LH ");
        break;
    case LW:
        *rd = mem_rw(rs1 + imm);
        printf("LW ");
        break;
    case LBU:
        *rd = (unsigned)mem_rb(rs1 + imm);
        printf("LBU ");
        break;
    case LHU:
        *rd = (unsigned)mem_rh(rs1 + imm);
        printf("LHU ");
        break;
    default:
        fprintf(stderr, "%x IL-Type BADCODE\n", ins);
    }
    printf("rd: %s, rs1: %s, imm: %x\n", re_na(RD(ins)), re_na(RS1(ins)), imm);
    printf("content now:  %x, %x\n", core->regs[RD(ins)], core->regs[RS1(ins)]);
}

void vcore_s_type(VCore* core, uint32_t ins)
{
    uint32_t rs1 = core->regs[RS1(ins)], rs2 = core->regs[RS2(ins)];

    switch (FUNC(ins)) {
    case SB:
        mem_wb(rs1 + S_IMM(ins), rs2);
        printf("SB ");
        break;
    case SH:
        mem_wh(rs1 + S_IMM(ins), rs2);
        printf("SH ");
        break;
    case SW:
        mem_ww(rs1 + S_IMM(ins), rs2);
        printf("SW ");
        break;
    default:
        fprintf(stderr, "%x S-Type BADCODE\n", ins);
    }
    printf("rd: %s, rs1: %s, rs2: %s, imm: %x\n", re_na(RD(ins)), re_na(RS1(ins)), re_na(RS2(ins)), S_IMM(ins));
    printf("content now:  %x, %x\n", core->regs[RS1(ins)], core->regs[RS2(ins)]);
}
