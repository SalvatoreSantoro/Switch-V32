#include "vm.h"
#include "csr.h"
#include "defs.h"
#include "memory.h"
#include <assert.h>
#include <stdint.h>

#define GET_PPN1(ppn)                (ppn & 0x3FFC00u)
#define GET_PPN0(ppn)                (ppn & 0x3FFu)
#define GET_ADDR_LAST_10(addr)       (addr & (0xFFC00000u))
#define GET_VP_OFFSET(addr)          (addr & 0xFFFu)
#define GET_MEGAPAGE_VP_OFFSET(addr) (addr & 0x3FFFFFu)

#define TREE_WALK_TRAPPED (0xFFFFFFFFFFFFFFFFu)

typedef struct {
    uint32_t ppn : 22; // Physical Page Number bits [31:10]
    uint32_t rsw : 2;  // Reserved for software
    uint32_t d : 1;    // Dirty bit
    uint32_t a : 1;    // Accessed bit
    uint32_t g : 1;    // Global bit
    uint32_t u : 1;    // User bit
    uint32_t x : 1;    // Execute permission
    uint32_t w : 1;    // Write permission
    uint32_t r : 1;    // Read permission
    uint32_t v : 1;    // Valid bit
} PTE;

uint64_t vm_tree_walk(VCore *core, uint32_t addr, op_type op) {
    // assert virtual memory is active
    assert(SATP_MODE(core->satp));
    // root page_addr
    uint32_t pp_addr = SATP_PPN(core->satp) * PAGE_SIZE;
    assert(pp_addr < MEM_SIZE);

    uint32_t tmp_addr = addr;

    // physical address could be up to 34 bits
    uint64_t final_paddr;

    // 4096 PTE
    PTE *pte_array;
    PTE *selected_pte;
    for (short level = 1; level >= 0; level--) {
        // directly access the memory to reduce overhead
        // assume aligned to stop clang warning (Wcast-align) since
        // we're sure that root_page_addr is 4096 bytes aligned
        pte_array = (PTE *) (__builtin_assume_aligned(g_mem.m + pp_addr, 4));
        selected_pte = pte_array + (GET_ADDR_LAST_10(tmp_addr) * sizeof(PTE));

        // not valid or reserved combination
        if ((selected_pte->v == 0) || (selected_pte->r == 0 && selected_pte->w == 1)) {
            dispatch_trap(core, (trap_code) op, addr);
            return TREE_WALK_TRAPPED;
        }

        // pte is valid
        if ((selected_pte->r == 1) || (selected_pte->x == 1)) {
            // leaf pte

            // if SSTATUS_MXR is set and we're reading, we're basically guaranteed to succeed (the outer if already
            // check read or executable)
            if (((!SSTATUS_MXR(core->sstatus)) && (op == MEM_READ) && (selected_pte->r == 0)) ||
                ((op == MEM_INS_FETCH) && (selected_pte->x == 0)) || ((op == MEM_WRITE) && (selected_pte->w == 0))) {
                dispatch_trap(core, (trap_code) op, addr);
                return TREE_WALK_TRAPPED;
            }

            // check mode privilege
            // is SSTATUS_SUM is set we disable the check on the "U" PTEs when executing in supervisor mode
            // but only if we're not fetching instruction, Supervisor can't never execute User code
            if (((!SSTATUS_SUM(core->sstatus) || (op == MEM_INS_FETCH)) && (selected_pte->u == 1) &&
                 (core->mode == SUPERVISOR_MODE)) ||
                ((selected_pte->u == 0) && (core->mode == USER_MODE))) {
                dispatch_trap(core, (trap_code) op, addr);
                return TREE_WALK_TRAPPED;
            }

            // MEGAPAGE
            if (level > 0) {
                // misaligned megapage
                if (GET_PPN0(selected_pte->ppn) != 0) {
                    dispatch_trap(core, (trap_code) op, addr);
                    return TREE_WALK_TRAPPED;
                }
            }

            // demanding the supervisor to set A and D bits
            if (selected_pte->a == 0 || ((selected_pte->d == 0) && (op == MEM_WRITE))) {
                dispatch_trap(core, (trap_code) op, addr);
                return TREE_WALK_TRAPPED;
            }

            // finished translating

            // MEGAPAGE
            if (level > 0) // 12 | 22 = 34 bits
                return (((uint64_t) GET_PPN1(selected_pte->ppn)) << 12) | (GET_MEGAPAGE_VP_OFFSET(addr));
            else
                return (((uint64_t) (selected_pte->ppn) << 12)) | GET_VP_OFFSET(addr);
        }

        // move to next level of the tree
        pp_addr = selected_pte->ppn * PAGE_SIZE;
        // next iteration should use next 10 last bits of addr
        tmp_addr <<= 10;
    }

    // didn't found anything
    dispatch_trap(core, (trap_code) op, addr);
    return TREE_WALK_TRAPPED;
}
