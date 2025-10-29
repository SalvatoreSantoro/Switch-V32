#ifndef _SV32_MACROS_H
#define _SV32_MACROS_H

//#define ALL_VERBOSE
// #define SYS_VERBOSE
//   #define LOADER_VERBOSE
//#define CPU_VERBOSE

#include <stdio.h>
#include <stdlib.h>

#ifdef ALL_VERBOSE
    #define CPU_VERBOSE
    #define SYS_VERBOSE
    #define LOADER_VERBOSE
#endif

#ifdef CPU_VERBOSE
    #define LOG_R(op)                                                                                                  \
        do {                                                                                                           \
            printf("%s rd: %s, rs1: %s, rs2: %s\n", op, re_na(RD(ins)), re_na(RS1(ins)), re_na(RS2(ins)));             \
            printf("Registers content:  %x, %x, %x\n\n", core->regs[RD(ins)], core->regs[RS1(ins)],                    \
                   core->regs[RS2(ins)]);                                                                              \
        } while (0)

    #define LOG_I(op, _imm)                                                                                            \
        do {                                                                                                           \
            printf("%s rd: %s, rs1: %s, imm: %x\n", op, re_na(RD(ins)), re_na(RS1(ins)), _imm);                        \
            printf("Registers content:  %x, %x\n\n", core->regs[RD(ins)], core->regs[RS1(ins)]);                       \
        } while (0)

    #define LOG_B(op, _imm)                                                                                            \
        do {                                                                                                           \
            printf("%s rs1: %s, rs2: %s, imm: %x\n", op, re_na(RS1(ins)), re_na(RS2(ins)), _imm);                      \
            printf("Registers content:  %x, %x\n\n", core->regs[RS1(ins)], core->regs[RS2(ins)]);                      \
        } while (0)

    #define LOG_J()                                                                                                    \
        do {                                                                                                           \
            printf("J rd: %s, imm: %x\n\n", re_na(RD(ins)), J_IMM(ins));                                               \
        } while (0)

    #define LOG_IJ()                                                                                                   \
        do {                                                                                                           \
            printf("IJ rd: %s, rs1: %s , imm: %x\n\n", re_na(RD(ins)), re_na(RS1(ins)), I_IMM(ins));                   \
        } while (0)
    #define LOG_LUI()                                                                                                  \
        do {                                                                                                           \
            printf("LUI rd: %s, imm: %x\n", re_na(RD(ins)), U_IMM(ins));                                               \
            printf("Register content: %x\n\n", core->regs[RD(ins)]);                                                   \
        } while (0)
    #define LOG_AUIPC()                                                                                                \
        do {                                                                                                           \
            printf("AUIPC rd: %s, imm: %x\n", re_na(RD(ins)), U_IMM(ins));                                             \
            printf("Register content: %x\n\n", core->regs[RD(ins)]);                                                   \
        } while (0)
    #define LOG_S(op)                                                                                                  \
        do {                                                                                                           \
            printf("%s rs1: %s, rs2: %s, imm: %x\n", op, re_na(RS1(ins)), re_na(RS2(ins)), S_IMM(ins));                \
            printf("Register content:  %x, %x\n\n", core->regs[RS1(ins)], core->regs[RS2(ins)]);                       \
        } while (0)

#else
    #define LOG_R(op)
    #define LOG_I(op, _imm)
    #define LOG_B(op, _imm)
    #define LOG_J()
    #define LOG_IJ()
    #define LOG_LUI()
    #define LOG_AUIPC()
    #define LOG_S(op)
#endif

#ifdef SYS_VERBOSE
    #define LOG_DE(op, d)  printf("%s: %u\n", op, d);
    #define LOG_EX(op, x)  printf("%s: %x\n", op, x);
    #define LOG_STR(op, s) printf("%s: %d\n", op, s);
#else
    #define LOG_DE(op, d)
    #define LOG_EX(op, x)
    #define LOG_STR(op, s)
#endif

#ifdef LOADER_VERBOSE
    #define LOG_LOAD()                                                                                                 \
        printf("Loaded Segment number %d, with addr %x and size %x in %p\n", i + 1, addr, memsz, MAP_ADDR(addr));
#else
    #define LOG_LOAD()
#endif

#define SV32_CRASH(msg)                                                                                                \
    do {                                                                                                               \
        fprintf(stderr, "[SV32_CRASH] %s:%d: %s\n", __FILE__, __LINE__, (msg));                                        \
        fflush(stderr);                                                                                                \
        exit(EXIT_FAILURE);                                                                                            \
    } while (0)

#define SV32_EXIT(str)                                                                                                 \
    do {                                                                                                               \
        printf("%s\n", str);                                                                                           \
        exit(EXIT_SUCCESS);                                                                                            \
    } while (0)

#endif
