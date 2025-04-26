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

// Gets
#define RD(x) ((x & IM_RD_MASK) >> 7)
#define RS1(x) ((x & RS1_MASK) >> 15)
#define RS2(x) ((x & RS2_MASK) >> 20)
#define R_FUNC(x) (((x & F3_MASK) >> 12) | ((x & IM2_F7_MASK) >> 21))
#define FUNC(x) ((x & F3_MASK) >> 12)

// R type
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

void vcore_r_type(VCore* core, uint32_t ins)
{
    uint32_t rs1 = core->regs[RS1(ins)], rs2 = core->regs[RS2(ins)];
    uint32_t* rd = &core->regs[RD(ins)];
    switch (R_FUNC(ins)) {
    case ADD:
        *rd = rs1 + rs2;
        //printf("ADD rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case SUB:
        *rd = rs1 - rs2;
        //printf("SUB rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case XOR:
        *rd = rs1 ^ rs2;
        //printf("XOR rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case OR:
        *rd = rs1 | rs2;
        //printf("OR rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case AND:
        *rd = rs1 & rs2;
        //printf("AND rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case SLL:
        *rd = rs1 << rs2;
        //printf("SLL rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case SRL:
        *rd = (unsigned)rs1 >> rs2;
        //printf("SRL rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case SRA:
        *rd = (signed)rs1 >> rs2;
        //printf("SRA rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case SLT:
        *rd = ((signed)rs1 < (signed)rs2) ? 1 : 0;
        //printf("SLT rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    case SLTU:
        *rd = ((unsigned)rs1 < (unsigned)rs2) ? 1 : 0;
        //printf("SLTU rd: %d, rs1 %d, rs2 %d\n", RD(ins), rs1, rs2);
        break;
    default:
        fprintf(stderr, "%x R-Type BADCODE\n", ins);
    }
}
