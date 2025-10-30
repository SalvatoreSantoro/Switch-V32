#ifndef SV32_VM_H
#define SV32_VM_H

#include "cpu.h"
#include "exception.h"

// match the exception codes for ease of use
typedef enum {
    MEM_INS_FETCH = INS_PAGE_FAULT,
    MEM_READ = LOAD_PAGE_FAULT,
	MEM_WRITE = STORE_AMO_PAGE_FAULT,
} op_type;


uint64_t vm_tree_walk(VCore *core, uint32_t addr, op_type op);

#endif
