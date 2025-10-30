#ifndef SV32_CSR_H
#define SV32_CSR_H

#define SATP_MODE(satp) (!!(satp & (1u << 31)))
#define SATP_PPN(satp)  (satp & (0x3FFFFFu))

#define SSTATUS_MXR(sstatus) (!!((sstatus) & (1u << 19)))
#define SSTATUS_SUM(sstatus) (!!((sstatus) & (1u << 18)))

#endif
