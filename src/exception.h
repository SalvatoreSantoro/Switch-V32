#ifndef SV32_EXCEPTION_H
#define SV32_EXCEPTION_H

typedef enum {
    INS_ADDR_MIS_ALGN = 0,
    INS_ACC_FAULT = 1,
    ILL_INS = 2,
    BRKPT = 3,
    LOAD_ADDR_MIS_ALGN = 4,
    LOAD_ACC_FAULT = 5,
    STORE_AMO_ADDR_MIS_ALGN = 6,
    STORE_AMO_ACC_FAULT = 7,
    ECALL_U_MODE = 8,
    ECALL_S_MODE = 9,
    INS_PAGE_FAULT = 12,
    LOAD_PAGE_FAULT = 13,
    STORE_AMO_PAGE_FAULT = 15
} exception_code;

void dispatch_exception(exception_code code);

#endif
