#ifndef SV32_CSR_H
#define SV32_CSR_H

#define SATP_MODE(satp) (!!(satp & (1u << 31)))
#define SATP_PPN(satp)  (satp & (0x3FFFFFu))

#define SET_SSTATUS_SIE(sstatus)    (sstatus | (1u << 1))
#define RESET_SSTATUS_SIE(sstatus)  (sstatus & ~(1u << 1))
#define SET_SSTATUS_SPIE(sstatus)   (sstatus | (1u << 5))
#define RESET_SSTATUS_SPIE(sstatus) (sstatus & ~(1u << 5))
#define SET_SSTATUS_SPP(sstatus)    (sstatus | (1u << 8))
#define RESET_SSTATUS_SPP(sstatus)  (sstatus & ~(1u << 8))

#define SSTATUS_SIE(sstatus) (!!((sstatus) & (1u << 1)))
#define SSTATUS_SPP(sstatus) (!!((sstatus) & (1u << 8)))
#define SSTATUS_MXR(sstatus) (!!((sstatus) & (1u << 19)))
#define SSTATUS_SUM(sstatus) (!!((sstatus) & (1u << 18)))

#define SIP_SSIP(sip) (!!((sip) & (1u << 1)))
#define SIP_STIP(sip) (!!((sip) & (1u << 5)))
#define SIP_SEIP(sip) (!!((sip) & (1u << 9)))

#define SIE_SSIE(sie) (!!((sie) & (1u << 1)))
#define SIE_STIE(sie) (!!((sie) & (1u << 5)))
#define SIE_SEIE(sie) (!!((sie) & (1u << 9)))

#define STVEC_MODE(stvec) (stvec & (0x3u))
#define STVEC_BASE(stvec) (stvec & (0xFFFFFFFCu))

#define STVEC_MODE_DIRECT   0
#define STVEC_MODE_VECTORED 1

#endif
