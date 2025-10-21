#include "cpu.h"
#include "emu.h"
#include "macros.h"
#include "memory.h"
#include "threads_mgr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

extern Threads_Mgr threads_mgr;

// Base Integer

// Masks
#define IM_RD_MASK  (0b11111 << 7)
#define RS1_MASK    (0b11111 << 15)
#define RS2_MASK    (0b11111 << 20)
#define IM2_F7_MASK (0b1111111 << 25)
#define F3_MASK     (0b111 << 12)
#define F5_MASK     (0b11111 << 27)
#define R_IMM_MASK  (0b111111111111 << 20)
#define U_IMM_MASK  (0b11111111111111111111 << 12)

// Instruction Decoders
#define RD(x)     ((x & IM_RD_MASK) >> 7)
#define RS1(x)    ((x & RS1_MASK) >> 15)
#define RS2(x)    ((x & RS2_MASK) >> 20)
#define R_FUNC(x) (((x & F3_MASK) >> 12) | ((x & IM2_F7_MASK) >> 21))
#define A_FUNC(x) ((x & F3_MASK >> 12) | ((x & F5_MASK) >> 23))
#define FUNC(x)   ((x & F3_MASK) >> 12)

#define I_IMM(x) ((int32_t) (x & R_IMM_MASK) >> 20)

#define B_IMM(x)                                                                                                       \
    ((((0b1111 << 8) & x) >> 7) | (((0b111111 << 25) & x) >> 20) | (((0b1 << 7) & x) << 4) |                           \
     ((int32_t) ((0b1 << 31) & x) >> 19))

#define J_IMM(x)                                                                                                       \
    (((((x) >> 21) & 0b1111111111) << 1) | ((((x) >> 20) & 0b1) << 11) | ((((x) >> 12) & 0b11111111) << 12) |          \
     (((int32_t) (x) >> 11) & 0xFFF00000))

#define U_IMM(x) (x & (U_IMM_MASK))
#define S_IMM(x) ((RD(x)) | ((int32_t) (x & IM2_F7_MASK) >> 20))

// R / IR type
// func7:func3
#define ADD  (0x000)
#define SUB  (0x200)
#define XOR  (0x004)
#define OR   (0x006)
#define AND  (0x007)
#define SLL  (0x001)
#define SRL  (0x005)
#define SRA  (0x205)
#define SLT  (0x002)
#define SLTU (0x003)

#define MUL    (0x010)
#define MULH   (0x011)
#define MULHSU (0x012)
#define MULHU  (0x013)
#define DIV    (0x014)
#define DIVU   (0x015)
#define REM    (0x016)
#define REMU   (0x017)

// B Type
// func3
#define BEQ  (0x0)
#define BNE  (0x1)
#define BLT  (0x4)
#define BGE  (0x5)
#define BLTU (0x6)
#define BGEU (0x7)

// IL Type
// func3
#define LB  (0x0)
#define LH  (0x1)
#define LW  (0x2)
#define LBU (0x4)
#define LHU (0x5)

// S Type
// func3
#define SB (0x0)
#define SH (0x1)
#define SW (0x2)

// A Type
// func5:func3
#define LR   (0x022)
#define SC   (0x032)
#define ASWP (0x012)
#define AADD (0x002)
#define AAND (0x0C2)
#define AOR  (0x0A2)
#define AXOR (0x042)
#define AMAX (0x142)
#define AMIN (0x102)

// E Type
#define ECALL  (0x0)
#define EBREAK (0x1)

const char *re_na(int reg_num) {
    switch (reg_num) {
#define X(name, value)                                                                                                 \
    case value:                                                                                                        \
        return #name;
        REG_LIST
#undef X
    default:
        return "UNKNOWN_REG";
    }
}

#define INSTR_SWITCH                                                                                                   \
    do {                                                                                                               \
        if (IS_COMPRESSED(ins)) {                                                                                      \
            pc_next = core->pc + 2;                                                                                    \
            printf("Compressed unimplemented\n");                                                                      \
            exit(EXIT_FAILURE);                                                                                        \
        } else {                                                                                                       \
            pc_next = core->pc + 4;                                                                                    \
            switch (OPCODE_TYPE(ins)) {                                                                                \
            case R_TYPE:                                                                                               \
                vcore_r_type(core, ins);                                                                               \
                break;                                                                                                 \
            case IR_TYPE:                                                                                              \
                vcore_ir_type(core, ins);                                                                              \
                break;                                                                                                 \
            case IL_TYPE:                                                                                              \
                vcore_il_type(core, ins);                                                                              \
                break;                                                                                                 \
            case S_TYPE:                                                                                               \
                vcore_s_type(core, ins);                                                                               \
                break;                                                                                                 \
            case B_TYPE:                                                                                               \
                pc_next = vcore_b_type(core, ins);                                                                     \
                break;                                                                                                 \
            case J_TYPE:                                                                                               \
                pc_next = vcore_j_type(core, ins);                                                                     \
                break;                                                                                                 \
            case IJ_TYPE:                                                                                              \
                pc_next = vcore_ij_type(core, ins);                                                                    \
                break;                                                                                                 \
            case LUI:                                                                                                  \
                vcore_lui_type(core, ins);                                                                             \
                break;                                                                                                 \
            case AUIPC:                                                                                                \
                vcore_auipc_type(core, ins);                                                                           \
                break;                                                                                                 \
            case A_TYPE:                                                                                               \
                vcore_a_type(core, ins);                                                                               \
                break;                                                                                                 \
            case ENV_TYPE:                                                                                             \
                if (vcore_e_type(core, ins)) { /* if true(breakpoint) don't update the PC */                           \
                    pc_next = core->pc;                                                                                \
                }                                                                                                      \
                break;                                                                                                 \
            default:                                                                                                   \
                fprintf(stderr, "%x BADOPCODE at %x\n", ins, core->pc);                                                \
                exit(EXIT_FAILURE);                                                                                    \
                break;                                                                                                 \
            }                                                                                                          \
            core->pc = pc_next;                                                                                        \
        }                                                                                                              \
    } while (0)

void vcore_r_type(VCore *core, uint32_t ins) {
    uint32_t rs1 = core->regs[RS1(ins)], rs2 = core->regs[RS2(ins)];
    uint32_t *rd = &core->regs[RD(ins)];
    uint32_t func = R_FUNC(ins);
    uint64_t tmp_mul;
    // Divide by zero
    if (rs2 == 0) {
        if ((func == DIV) || (func == DIVU)) {
            *rd = 0xFFFFFFFF;
            return;
        }
        if ((func == REM) || (func == REMU)) {
            *rd = rs1;
            return;
        }
    }
    // overlow (minnegative / -1)
    if (((signed) rs2 == -1) && ((signed) rs1 == INT32_MIN)) {
        if ((func == DIV) || (func == DIVU)) {
            *rd = INT32_MIN;
            return;
        }
        if ((func == REM) || (func == REMU)) {
            *rd = 0;
            return;
        }
    }

    switch (func) {
    case ADD:
        *rd = rs1 + rs2;
        LOG_R("ADD");
        break;
    case SUB:
        *rd = rs1 - rs2;
        LOG_R("SUB");
        break;
    case XOR:
        *rd = rs1 ^ rs2;
        LOG_R("XOR");
        break;
    case OR:
        *rd = rs1 | rs2;
        LOG_R("OR");
        break;
    case AND:
        *rd = rs1 & rs2;
        LOG_R("AND");
        break;
    case SLL:
        *rd = (uint32_t) rs1 << rs2;
        LOG_R("SLL");
        break;
    case SRL:
        *rd = (uint32_t) rs1 >> rs2;
        LOG_R("SRL");
        break;
    case SRA:
        *rd = (int32_t) rs1 >> rs2;
        LOG_R("SRA");
        break;
    case SLT:
        *rd = ((int32_t) rs1 < (int32_t) rs2) ? 1 : 0;
        LOG_R("SLT");
        break;
    case SLTU:
        *rd = ((uint32_t) rs1 < (uint32_t) rs2) ? 1 : 0;
        LOG_R("SLTU");
        break;
    case MUL:
        *rd = (int32_t) rs1 * (int32_t) rs2;
        LOG_R("MUL");
        break;
    case MULH:
        tmp_mul = (int64_t) (int32_t) rs1 * (int64_t) (int32_t) rs2;
        tmp_mul &= (0xFFFFFFFF00000000);
        *rd = (uint32_t) (tmp_mul >> 32);
        LOG_R("MULH");
        break;
    case MULHSU:
        tmp_mul = (int64_t) (int32_t) rs1 * (uint64_t) (uint32_t) rs2;
        tmp_mul &= (0xFFFFFFFF00000000);
        *rd = (uint32_t) (tmp_mul >> 32);
        LOG_R("MULHSU");
        break;
    case MULHU:
        tmp_mul = (uint64_t) (uint32_t) rs1 * (uint64_t) (uint32_t) rs2;
        tmp_mul &= (0xFFFFFFFF00000000);
        *rd = (uint32_t) (tmp_mul >> 32);
        LOG_R("MULHU");
        break;
    case DIV:
        *rd = (int32_t) rs1 / (int32_t) rs2;
        LOG_R("DIV");
        break;
    case DIVU:
        *rd = (uint32_t) rs1 / (uint32_t) rs2;
        LOG_R("DIVU");
        break;
    case REM:
        *rd = (int32_t) rs1 % (int32_t) rs2;
        LOG_R("REM");
        break;
    case REMU:
        *rd = (uint32_t) rs1 % (uint32_t) rs2;
        LOG_R("REMU");
        break;
    default:
        fprintf(stderr, "%x R-Type BADCODE\n", ins);
        exit(EXIT_FAILURE);
    }
}

void vcore_ir_type(VCore *core, uint32_t ins) {
    uint32_t rs1 = core->regs[RS1(ins)];
    uint32_t *rd = &core->regs[RD(ins)];
    int func = R_FUNC(ins);
    int32_t imm;
    // need to be unsigned
    uint32_t shift_imm = (uint32_t) RS2(ins);

    if (func == SRA) {
        *rd = (int32_t) rs1 >> shift_imm;
        LOG_I("SRA", shift_imm);
        return;
    }

    if (func == SRL) {
        *rd = (uint32_t) rs1 >> shift_imm;
        LOG_I("SRL", shift_imm);
        return;
    }
    if (func == SLL) {
        *rd = (uint32_t) rs1 << shift_imm;
        LOG_I("SLL", shift_imm);
        return;
    }

    imm = I_IMM(ins);
    func = FUNC(ins);

    switch (func) {
    case ADD:
        *rd = rs1 + imm;
        LOG_I("ADD", imm);
        break;
    case XOR:
        *rd = rs1 ^ imm;
        LOG_I("XOR", imm);
        break;
    case OR:
        *rd = rs1 | imm;
        LOG_I("OR", imm);
        break;
    case AND:
        *rd = rs1 & imm;
        LOG_I("AND", imm);
        break;
    case SLT:
        *rd = ((int32_t) rs1 < (int32_t) imm) ? 1 : 0;
        LOG_I("SLT", imm);
        break;
    case SLTU:
        *rd = ((uint32_t) rs1 < (uint32_t) imm) ? 1 : 0;
        LOG_I("SLTU", imm);
        break;
    default:
        fprintf(stderr, "%x IR-Type BADCODE\n", ins);
        exit(EXIT_FAILURE);
    }
}

uint32_t vcore_b_type(VCore *core, uint32_t ins) {
    uint32_t rs1 = core->regs[RS1(ins)], rs2 = core->regs[RS2(ins)];
    int32_t inc = 4;
    int32_t imm = B_IMM(ins);
    switch (FUNC(ins)) {
    case BEQ:
        if (rs1 == rs2)
            inc = imm;
        LOG_B("BEQ", imm);
        break;
    case BNE:
        if (rs1 != rs2)
            inc = imm;
        LOG_B("BNE", imm);
        break;
    case BLT:
        if ((int32_t) rs1 < (int32_t) rs2)
            inc = imm;
        LOG_B("BLT", imm);
        break;
    case BGE:
        if ((int32_t) rs1 >= (int32_t) rs2)
            inc = imm;
        LOG_B("BGE", imm);
        break;
    case BLTU:
        if ((uint32_t) rs1 < (uint32_t) rs2)
            inc = imm;
        LOG_B("BLTU", imm);
        break;
    case BGEU:
        if ((uint32_t) rs1 >= (uint32_t) rs2)
            inc = imm;
        LOG_B("BGEU", imm);
        break;
    default:
        fprintf(stderr, "%x B-Type BADCODE\n", ins);
        exit(EXIT_FAILURE);
    }
    return core->pc + inc;
}

inline uint32_t vcore_j_type(VCore *core, uint32_t ins) {
    LOG_J();
    core->regs[RD(ins)] = core->pc + 4;
    return core->pc + J_IMM(ins);
}

inline uint32_t vcore_ij_type(VCore *core, uint32_t ins) {
    LOG_IJ();
    core->regs[RD(ins)] = core->pc + 4;
    return core->regs[RS1(ins)] + I_IMM(ins);
}

inline void vcore_lui_type(VCore *core, uint32_t ins) {
    core->regs[RD(ins)] = U_IMM(ins);
    LOG_LUI();
}

inline void vcore_auipc_type(VCore *core, uint32_t ins) {
    core->regs[RD(ins)] = core->pc + U_IMM(ins);
    LOG_AUIPC();
}

void vcore_il_type(VCore *core, uint32_t ins) {
    uint32_t rs1 = core->regs[RS1(ins)];
    uint32_t *rd = &core->regs[RD(ins)];
    int32_t imm = I_IMM(ins);
    // LB and LH need sign extend
    switch (FUNC(ins)) {
    case LB:
        *rd = mem_rb(rs1 + imm);
        if (*rd >= 0b10000000)
            *rd |= 0xFFFFFF00;
        LOG_I("LB", imm);
        break;
    case LH:
        *rd = mem_rh(rs1 + imm);
        if (*rd >= 0b1000000000000000)
            *rd |= 0xFFFF0000;
        LOG_I("LH", imm);
        break;
    case LW:
        // printf("ADDR: %x\n", __vmem.m + rs1 + imm);
        *rd = (uint32_t) mem_rw(rs1 + imm);
        LOG_I("LW", imm);
        break;
    case LBU:
        LOG_I("LBU", imm);
        *rd = (uint32_t) mem_rb(rs1 + imm);
        break;
    case LHU:
        *rd = (uint32_t) mem_rh(rs1 + imm);
        LOG_I("LHU", imm);
        break;
    default:
        fprintf(stderr, "%x IL-Type BADCODE\n", ins);
        exit(EXIT_FAILURE);
    }
}

void vcore_s_type(VCore *core, uint32_t ins) {
    uint32_t rs1 = core->regs[RS1(ins)], rs2 = core->regs[RS2(ins)];

    switch (FUNC(ins)) {
    case SB:
        mem_wb(rs1 + S_IMM(ins), rs2);
        LOG_S("SB");
        break;
    case SH:
        mem_wh(rs1 + S_IMM(ins), rs2);
        LOG_S("SH");
        break;
    case SW:
        mem_ww(rs1 + S_IMM(ins), rs2);
        LOG_S("SW");
        break;
    default:
        fprintf(stderr, "%x S-Type BADCODE\n", ins);
        exit(EXIT_FAILURE);
    }
}

// acquire-release logic unimplemented for now, cause this implementation is
// single threaded
void vcore_a_type(VCore *core, uint32_t ins) {
    uint32_t rs1 = core->regs[RS1(ins)], rs2 = core->regs[RS2(ins)];
    uint32_t *rd = &core->regs[RD(ins)];
    uint32_t tmp_swp;
    uint32_t tmp_load;

    switch (A_FUNC(ins)) {
    case LR:
        core->reserved = rs1;
        tmp_load = mem_rw(rs1);
        // sign extension
        // 8 bit
        if ((tmp_load < 256) && (tmp_load > 127))
            tmp_load |= 0xFFFFFF00;
        // 16 bit
        else if ((tmp_load < 65535) && (tmp_load > 32767))
            tmp_load |= 0xFFFF0000;
        *rd = tmp_load;
        break;
    case SC:
        if (rs1 == core->reserved) {
            mem_ww(rs1, rs2);
            *rd = 0;
        } else
            *rd = 1;
        break;
    case ASWP:
        tmp_swp = mem_rw(rs1);
        *rd = rs2;
        core->regs[RS2(ins)] = tmp_swp;
        mem_ww(rs1, *rd);
        break;
    case AADD:
        *rd = (mem_rw(rs1) + rs2);
        mem_ww(rs1, *rd);
        break;
    case AAND:
        *rd = (mem_rw(rs1) & rs2);
        mem_ww(rs1, *rd);
        break;
    case AOR:
        *rd = (mem_rw(rs1) | rs2);
        mem_ww(rs1, *rd);
        break;
    case AXOR:
        *rd = (mem_rw(rs1) ^ rs2);
        mem_ww(rs1, *rd);
        break;
    case AMAX:
        tmp_load = mem_rw(rs1);
        *rd = (tmp_load > rs2 ? tmp_load : rs2);
        mem_ww(rs1, *rd);
        break;
    case AMIN:
        tmp_load = mem_rw(rs1);
        *rd = (tmp_load < rs2 ? tmp_load : rs2);
        mem_ww(rs1, *rd);
        break;
    default:
        fprintf(stderr, "%x A-Type BADCODE\n", ins);
        exit(EXIT_FAILURE);
    }
}

bool vcore_e_type(VCore *core, uint32_t ins) {
    int32_t imm = I_IMM(ins);
    switch (imm) {
    case ECALL:
        emu_system_call(core);
        break;
    case EBREAK:
        threads_mgr_halt_all();
        return true;
        break;
    default:
        fprintf(stderr, "%x E-Type BADCODE\n", ins);
        exit(EXIT_FAILURE);
    }
    return false;
}

void vcore_run(VCore *core) {
    uint32_t ins;
    uint32_t pc_next; // used inside INSTR_SWITCH macro

    while (1) {
        // reset ZERO reg at every iteration
        core->regs[ZERO] = 0;
        ins = mem_rw(core->pc);
        INSTR_SWITCH;

//When running with SUPERVISOR enabled exit check when to exit from the loop (SBI_EXT_HSM)
#ifdef SUPERVISOR
		if (__atomic_load_n(&GET_HALT(core->core_idx).halted, __ATOMIC_ACQUIRE))
			return;
#endif
    }
}

void vcore_step(VCore *core) {
    uint32_t ins;
    uint32_t pc_next; // used inside INSTR_SWITCH macro

    core->regs[ZERO] = 0;
    ins = mem_rw(core->pc);
    INSTR_SWITCH;
}
