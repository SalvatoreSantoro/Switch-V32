#include "cpu.h"
#include "csr.h"
#include "sbi.h"
#include "trap.h"
#include <stdint.h>

#define CSRRW  0x001
#define CSRRS  0x010
#define CSRRC  0x011
#define CSRRWI 0x101
#define CSRRSI 0x110
#define CSRRCI 0x111

#define SSTATUS_ADDR    0x100
#define SIE_ADDR        0x104
#define STVEC_ADDR      0x105
#define SCOUNTEREN_ADDR 0x106
#define SENVCFG_ADDR    0x10B
#define SSCRATCH_ADDR   0x140
#define SEPC_ADDR       0x141
#define SCAUSE_ADDR     0x142
#define STVAL_ADDR      0x143
#define SIP_ADDR        0x144
#define SATP_ADDR       0x180

#define CSR_READ_ACCESS          (0xC00u)
#define GET_CSR_ACCESS(csr_addr) (csr_addr & (0xC00u))

#define CSR_SUPERVISOR_LEVEL (0x100u)
#define CSR_USER_LEVEL       (0x000u)

#define GET_CSR_PRIVILEGE(csr_addr) (csr_addr & (0x300u))

void vcore_sys_type(VCore *core, uint32_t ins) {
    uint32_t func = FUNC(ins);
    uint32_t imm = I_IMM(ins);

    if (func == 0) { // ENV
        switch (imm) {
        case ECALL:
            // supervisor ecall
            if (core->mode == SUPERVISOR_MODE)
                emu_sbi_call(core);
            // user ecall
            else
                dispatch_trap(core, ECALL_U_MODE, core->pc);

            core->pc += 4;
            break;
        case EBREAK:
            // never increase PC, we want that the core stops on the breakpoint
            // if debugging. If the breakpoint is an exception INSIDE an USER program
            // running on a SUPERVISOR, we also don't want to increment because the dispatch exception
            // will change the PC so that the next instruction is the first of the exception handler
            dispatch_trap(core, BRKPT, core->pc);
            break;
        default:
            dispatch_trap(core, ILL_INS, ins);
            break;
        }
    } else { // CSR
        uint32_t csr_addr = imm;
        uint32_t rs1_val = core->regs[RS1(ins)];
        uint32_t rs1_imm = RS1(ins);
        uint32_t *rd = &core->regs[RD(ins)];
        uint32_t *csr;

        if (GET_CSR_PRIVILEGE(csr_addr) == CSR_SUPERVISOR_LEVEL) {
            // privilege control
            if (core->mode != SUPERVISOR_MODE) {
                dispatch_trap(core, ILL_INS, ins);
                return;
            }
            // find the csr
            switch (csr_addr) {
            case SSTATUS_ADDR:
                csr = &core->sstatus;
                break;
            case SIE_ADDR:
                csr = &core->sie;
                break;
            case STVEC_ADDR:
                csr = &core->stvec;
                break;
            case SCOUNTEREN_ADDR:
                csr = &core->scounteren;
                break;
            case SENVCFG_ADDR:
                csr = &core->senvcfg;
                break;
            case SSCRATCH_ADDR:
                csr = &core->sscratch;
                break;
            case SEPC_ADDR:
                csr = &core->sepc;
                break;
            case SCAUSE_ADDR:
                csr = &core->scause;
                break;
            case STVAL_ADDR:
                csr = &core->stval;
                break;
            case SIP_ADDR:
                csr = &core->sip;
                break;
            case SATP_ADDR:
                csr = &core->satp;
                break;
            default:
                // unknown register
                dispatch_trap(core, ILL_INS, ins);
                return;
                break;
            }
        } else if (GET_CSR_PRIVILEGE(csr_addr) == CSR_USER_LEVEL) {
            // implement user level
        } else {
            // unknown register
            dispatch_trap(core, ILL_INS, ins);
            return;
        }

        // check writes on read-only register
        if (GET_CSR_ACCESS(csr_addr) == CSR_READ_ACCESS) {
            // if read-set or read-clear without zero register as rs1
            if (((func == CSRRS) || (func == CSRRC)) && (RS1(ins) != ZERO)) {
                dispatch_trap(core, ILL_INS, ins);
                return;
            }

            // if read-set-immediate or read-clear-immediate without zero immediate
            if (((func == CSRRSI) || (func == CSRRCI)) && (rs1_val != 0)) {
                dispatch_trap(core, ILL_INS, ins);
                return;
            }
        }

        // the only bit writable to SIP from csr instructions is SSIP
        if (csr_addr == SIP_ADDR) {
            rs1_val &= (1u << 1);
            rs1_imm &= (1u << 1);
        }

        // only SSIE, STIE and SEIE are writable
        if (csr_addr == SIE_ADDR) {
            rs1_val &= ((1u << 9) | (1u << 5) | (1u << 1));
            rs1_imm &= ((1u << 9) | (1u << 5) | (1u << 1));
        }

        switch (func) {
        case CSRRW:
            *rd = *csr;
            *csr = rs1_val;
            break;

        case CSRRS:
            *rd = *csr;
            *csr |= rs1_val;
            break;

        case CSRRC:
            *rd = *csr;
            *csr &= ~rs1_val;
            break;

        case CSRRWI:
            *rd = *csr;
            *csr = rs1_imm;
            break;

        case CSRRSI:
            *rd = *csr;
            *csr |= rs1_imm;
            break;

        case CSRRCI:
            *rd = *csr;
            *csr &= ~rs1_imm;
            break;

        default:
            break;
        }

        // fix WARL

        // stvec "MODE" >= 2 is reserved
        if ((csr_addr == STVEC_ADDR) && (STVEC_MODE(*csr) >= 2)) {
            // force Direct
            *csr &= ~0x3u;
        }
    }
}
