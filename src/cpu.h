#ifndef _RISCV32_CPU_H
#define _RISCV32_CPU_H

#include <stdint.h>
// Register name to number mapping
#define REG_ZERO 0 // x0
#define REG_RA 1 // x1
#define REG_SP 2 // x2
#define REG_GP 3 // x3
#define REG_TP 4 // x4

#define REG_T0 5 // x5
#define REG_T1 6 // x6
#define REG_T2 7 // x7

#define REG_S0 8 // x8 (also known as FP)
#define REG_S1 9 // x9

#define REG_A0 10 // x10
#define REG_A1 11 // x11
#define REG_A2 12 // x12
#define REG_A3 13 // x13
#define REG_A4 14 // x14
#define REG_A5 15 // x15
#define REG_A6 16 // x16
#define REG_A7 17 // x17

#define REG_S2 18 // x18
#define REG_S3 19 // x19
#define REG_S4 20 // x20
#define REG_S5 21 // x21
#define REG_S6 22 // x22
#define REG_S7 23 // x23
#define REG_S8 24 // x24
#define REG_S9 25 // x25
#define REG_S10 26 // x26
#define REG_S11 27 // x27

#define REG_T3 28 // x28
#define REG_T4 29 // x29
#define REG_T5 30 // x30
#define REG_T6 31 // x31
#define PC 32 // x31

#define REG_NUMS 33

typedef struct {
    uint32_t regs[REG_NUMS];
} VCore;

VCore* vcore_create(void);

void vcore_destroy(VCore* core);

void vcore_load_register(VCore* core, int reg, uint32_t value);

#endif
