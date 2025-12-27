#include "cpu.h"
#include "macros.h"
#include "memory.h"
#include "threads_mgr2.h"
#include "trap.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

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
#define SYS_TYPE       0x73
#define A_TYPE         0x2F

// Every mask and decode assume to operate on an uint32_t "x" (instruction)
// so the result is always an uint32_t

// Base Integer

// Masks

#define RS2_MASK    (0x1Fu << 20)
#define IM2_F7_MASK (0x7Fu << 25)
#define F5_MASK     (0x1Fu << 27)
#define U_IMM_MASK  (0xFFFFFu << 12)

// Instruction Decoders

#define RS2(x)    (uint32_t) ((x & RS2_MASK) >> 20)
#define R_FUNC(x) (uint32_t) (((x & F3_MASK) >> 12) | ((x & IM2_F7_MASK) >> 21))
#define A_FUNC(x) (uint32_t) (((x & F3_MASK) >> 12) | ((x & F5_MASK) >> 23))

#define B_IMM(x)                                                                                                       \
    ((((uint32_t) (0xF << 8) & x) >> 7) | (((uint32_t) (0x3F << 25) & x) >> 20) | (((uint32_t) (0x1 << 7) & x) << 4) | \
     (uint32_t) ((int32_t) ((uint32_t) (1u << 31) & x) >> 19))

#define J_IMM(x)                                                                                                       \
    (((((x) >> 21) & (uint32_t) 0x3FF) << 1) | ((((x) >> 20) & (uint32_t) 0x1) << 11) |                                \
     ((((x) >> 12) & (uint32_t) 0xFF) << 12) | ((uint32_t) ((int32_t) (x) >> 11) & (uint32_t) 0xFFF00000))

#define U_IMM(x) (x & (U_IMM_MASK))
#define S_IMM(x) ((RD(x)) | (uint32_t) ((int32_t) (x & IM2_F7_MASK) >> 20))

#define AQ_BIT(x) (!!(x & (1u << 0x19)))
#define RL_BIT(x) (!!(x & (1u << 0x1A)))

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
#define LR    (0x022)
#define SC    (0x032)
#define ASWP  (0x012)
#define AADD  (0x002)
#define AAND  (0x0C2)
#define AOR   (0x0A2)
#define AXOR  (0x042)
#define AMAX  (0x142)
#define AMIN  (0x102)
#define AMAXU (0x1C2)
#define AMINU (0x182)

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

static void vcore_r_type(VCore *core, uint32_t ins) {
    uint32_t rs1 = core->regs[RS1(ins)], rs2 = core->regs[RS2(ins)];
    uint32_t *rd = &core->regs[RD(ins)];
    uint32_t func = R_FUNC(ins);
    uint64_t tmp_mul;

    // Divide by zero
    if (rs2 == 0) {
        if ((func == DIV) || (func == DIVU)) {
            *rd = 0xFFFFFFFF;
            goto r_increase_pc;
        }
        if ((func == REM) || (func == REMU)) {
            *rd = rs1;
            goto r_increase_pc;
        }
    }

    // overflow (minnegative / -1)
    if ((((signed) rs2) == -1) && (((signed) rs1) == INT32_MIN)) {
        if (func == DIV) {
            *rd = (uint32_t) INT32_MIN;
            goto r_increase_pc;
        }
        if (func == REM) {
            *rd = 0;
            goto r_increase_pc;
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
        // only first 5 bits should take effect
        *rd = (uint32_t) (rs1 << (rs2 & 31));
        LOG_R("SLL");
        break;
    case SRL:
        // only first 5 bits should take effect
        *rd = (uint32_t) rs1 >> (rs2 & 31);
        LOG_R("SRL");
        break;
    case SRA:
        // only first 5 bits should take effect
        *rd = (uint32_t) ((int32_t) rs1 >> (rs2 & 31));
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
        *rd = (uint32_t) ((int64_t) (int32_t) rs1 * (int64_t) (int32_t) rs2);
        LOG_R("MUL");
        break;
    case MULH:
        tmp_mul = (uint64_t) ((int64_t) (int32_t) rs1 * (int64_t) (int32_t) rs2);
        tmp_mul &= (0xFFFFFFFF00000000);
        *rd = (uint32_t) (tmp_mul >> 32);
        LOG_R("MULH");
        break;
    case MULHSU:
        // second operand unsigned so just extend directly to 64bit without
        // first converting to signed (int32_t) so the sign isn't extended
        tmp_mul = (uint64_t) ((int64_t) (int32_t) rs1 * (int64_t) rs2);
        tmp_mul &= (0xFFFFFFFF00000000);
        *rd = (uint32_t) (tmp_mul >> 32);
        LOG_R("MULHSU");
        break;
    case MULHU:
        tmp_mul = (uint64_t) rs1 * (uint64_t) rs2;
        tmp_mul &= (0xFFFFFFFF00000000);
        *rd = (uint32_t) (tmp_mul >> 32);
        LOG_R("MULHU");
        break;
    case DIV:
        *rd = (uint32_t) ((int32_t) rs1 / (int32_t) rs2);
        LOG_R("DIV");
        break;
    case DIVU:
        *rd = rs1 / rs2;
        LOG_R("DIVU");
        break;
    case REM:
        *rd = (uint32_t) ((int32_t) rs1 % (int32_t) rs2);
        LOG_R("REM");
        break;
    case REMU:
        *rd = rs1 % rs2;
        LOG_R("REMU");
        break;
    default:
        dispatch_trap(core, ILL_INS, ins);
    }

r_increase_pc:
    core->regs[PC] += 4;
    return;
}

static void vcore_ir_type(VCore *core, uint32_t ins) {
    uint32_t rs1 = core->regs[RS1(ins)];
    uint32_t *rd = &core->regs[RD(ins)];
    uint32_t func = R_FUNC(ins);
    uint32_t imm;
    // need to be unsigned
    uint32_t shift_imm = (RS2(ins) & 31);

    if (func == SRA) {
        *rd = (uint32_t) ((int32_t) rs1 >> shift_imm);
        LOG_I("SRA", shift_imm);
        goto ir_increase_pc;
    }

    if (func == SRL) {
        *rd = (uint32_t) rs1 >> shift_imm;
        LOG_I("SRL", shift_imm);
        goto ir_increase_pc;
    }
    if (func == SLL) {
        *rd = (uint32_t) rs1 << shift_imm;
        LOG_I("SLL", shift_imm);
        goto ir_increase_pc;
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
        *rd = (rs1 < imm) ? 1 : 0;
        LOG_I("SLTU", imm);
        break;
    default:
        dispatch_trap(core, ILL_INS, 0);
        break;
    }

ir_increase_pc:
    core->regs[PC] += 4;
    return;
}

// TODO: should check if we're jumping to misaligned address?
static void vcore_b_type(VCore *core, uint32_t ins) {
    uint32_t rs1 = core->regs[RS1(ins)], rs2 = core->regs[RS2(ins)];
    uint32_t inc = 4;
    uint32_t imm = B_IMM(ins);
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
        dispatch_trap(core, ILL_INS, ins);
        break;
    }
    core->regs[PC] += inc;
}

// TODO: should check if we're jumping to misaligned address?
static inline void vcore_j_type(VCore *core, uint32_t ins) {
    LOG_J();
    core->regs[RD(ins)] = core->regs[PC] + 4;
    core->regs[PC] += J_IMM(ins);
}

static inline void vcore_ij_type(VCore *core, uint32_t ins) {
    LOG_IJ();
    core->regs[RD(ins)] = core->regs[PC] + 4;
    core->regs[PC] = core->regs[RS1(ins)] + I_IMM(ins);
}

static inline void vcore_lui_type(VCore *core, uint32_t ins) {
    core->regs[RD(ins)] = U_IMM(ins);
    LOG_LUI();
    core->regs[PC] += 4;
}

static inline void vcore_auipc_type(VCore *core, uint32_t ins) {
    core->regs[RD(ins)] = core->regs[PC] + U_IMM(ins);
    LOG_AUIPC();
    core->regs[PC] += 4;
}

static void vcore_il_type(VCore *core, uint32_t ins) {
    uint32_t rs1 = core->regs[RS1(ins)];
    uint32_t *rd = &core->regs[RD(ins)];
    uint32_t imm = I_IMM(ins);
    // LB and LH need sign extend
    switch (FUNC(ins)) {
    case LB:
        LOG_I("LB", imm);
        *rd = mem_rb(rs1 + imm);
        if (*rd >= 0x80)
            *rd |= 0xFFFFFF00;
        break;
    case LH:
        LOG_I("LH", imm);
        *rd = mem_rh(rs1 + imm);
        if (*rd >= 0x8000)
            *rd |= 0xFFFF0000;
        break;
    case LW:
        LOG_I("LW", imm);
        *rd = (uint32_t) mem_rw(rs1 + imm);
        break;
    case LBU:
        LOG_I("LBU", imm);
        *rd = (uint32_t) mem_rb(rs1 + imm);
        break;
    case LHU:
        LOG_I("LHU", imm);
        *rd = (uint32_t) mem_rh(rs1 + imm);
        break;
    default:
        dispatch_trap(core, ILL_INS, ins);
        break;
    }
    core->regs[PC] += 4;
}

static void vcore_s_type(VCore *core, uint32_t ins) {
    uint32_t rs1 = core->regs[RS1(ins)], rs2 = core->regs[RS2(ins)];

    switch (FUNC(ins)) {
    case SB:
        mem_wb(rs1 + S_IMM(ins), (uint8_t) rs2);
        LOG_S("SB");
        break;
    case SH:
        mem_wh(rs1 + S_IMM(ins), (uint16_t) rs2);
        LOG_S("SH");
        break;
    case SW:
        mem_ww(rs1 + S_IMM(ins), rs2);
        LOG_S("SW");
        break;
    default:
        dispatch_trap(core, ILL_INS, ins);
        break;
    }

    core->regs[PC] += 4;
}

// NEED TO ENFORCE MEMORY ALIGNMENT
static void vcore_a_type(VCore *core, uint32_t ins) {
    uint32_t rs1 = core->regs[RS1(ins)], rs2 = core->regs[RS2(ins)];
    uint32_t *rd = &core->regs[RD(ins)];
    uint32_t tmp_load;
    int memorder = __ATOMIC_RELAXED;

    // find memory order
    if (AQ_BIT(ins) && RL_BIT(ins)) {
        memorder = __ATOMIC_ACQ_REL;
    } else {
        if (AQ_BIT(ins))
            memorder = __ATOMIC_ACQUIRE;
        if (RL_BIT(ins))
            memorder = __ATOMIC_RELEASE;
    }

    switch (A_FUNC(ins)) {
    case LR:
        core->ll_sc_flag = true;
        core->reserved = mem_aread(rs1, memorder);
        break;
    case SC:
        if (!core->ll_sc_flag) {
            // fail if it didn't do a LR previously
            *rd = 1;
            return;
        }
        core->ll_sc_flag = false;
        mem_cas(rs1, rs2, core->reserved, memorder);
        *rd = 0;
        break;
    case ASWP:
        *rd = mem_aswp(rs1, rs2, memorder);
        break;
    case AADD:
        *rd = mem_aadd(rs1, rs2, memorder);
        break;
    case AAND:
        *rd = mem_aand(rs1, rs2, memorder);
        break;
    case AOR:
        *rd = mem_aor(rs1, rs2, memorder);
        break;
    case AXOR:
        *rd = mem_axor(rs1, rs2, memorder);
        break;
    case AMAX:
        *rd = (uint32_t) mem_amax(rs1, (int32_t) rs2, memorder);
        break;
    case AMIN:
        *rd = (uint32_t) mem_amin(rs1, (int32_t) rs2, memorder);
        break;
    case AMAXU:
        *rd = mem_amaxu(rs1, rs2, memorder);
        break;
    case AMINU:
        *rd = mem_aminu(rs1, rs2, memorder);
        break;
    default:
        dispatch_trap(core, ILL_INS, ins);
        break;
    }
    core->regs[PC] += 4;
}

void vcore_run(VCore *core) {
    uint32_t ins;

    while (1) {
        // reset ZERO reg at every iteration
        core->regs[ZERO] = 0;
        ins = mem_rw(core->regs[PC]);

        if (IS_COMPRESSED(ins)) {
            printf("Compressed unimplemented\n");
            exit(EXIT_FAILURE);
        } else {
            switch (OPCODE_TYPE(ins)) {
            case R_TYPE:
                vcore_r_type(core, ins);
                break;
            case IR_TYPE:
                vcore_ir_type(core, ins);
                break;
            case IL_TYPE:
                vcore_il_type(core, ins);
                break;
            case S_TYPE:
                vcore_s_type(core, ins);
                break;
            case B_TYPE:
                vcore_b_type(core, ins);
                break;
            case J_TYPE:
                vcore_j_type(core, ins);
                break;
            case IJ_TYPE:
                vcore_ij_type(core, ins);
                break;
            case LUI:
                vcore_lui_type(core, ins);
                break;
            case AUIPC:
                vcore_auipc_type(core, ins);
                break;
            case A_TYPE:
                vcore_a_type(core, ins);
                break;
            case SYS_TYPE:
                vcore_sys_type(core, ins);
                break;
            default:
                dispatch_trap(core, ILL_INS, ins);
                break;
            }

            // made it sequential to avoid helgrind false positives
            if (__atomic_load_n(&core->atomic_exit_loop, __ATOMIC_ACQ_REL)) {
                return;
            }

// due to LL/SR semantics i think that implementation of them must be done
// without checkings for interrupts untill they both terminate, in this way a
// set of LL/SR instructions isn' interrupted with another thread executing on the core
// the same LL/SR pair of instruction generating sort of ABA problems on the reserved value
//(i'm emulating LL/SR with CAS)
//
// remeber that when checking for interrupts, if the mode of the core is in USER the SIE bit in sstatus
// is ignored, so basically the core SHOULD interrupt anyway

// When running with SUPERVISOR enabled exit check when to exit from the loop (SBI_EXT_HSM)
#ifdef SUPERVISOR
            check_interrupts(core);
#endif
        }
    }
}

void vcore_reset(VCore *core) {
    memset(core, 0, sizeof(VCore));
    core->mode = SUPERVISOR_MODE;
}

void vcore_init(VCore *core, uint32_t start_addr, uint32_t id, uint32_t opaque) {
    vcore_reset(core);
    core->regs[PC] = start_addr;
    core->regs[A0] = id;
    core->regs[A1] = opaque;
}
