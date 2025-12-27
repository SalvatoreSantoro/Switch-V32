#include <stdint.h>

#define NUM_CORES        4
#define SBI_HART_START   0u
#define SBI_HART_SUSPEND 3u

extern void _entry(void);

static inline long sbi_ecall(long ext, long fid, long arg0, long arg1, long arg2) {
    register long a0 asm("a0") = arg0;
    register long a1 asm("a1") = arg1;
    register long a2 asm("a2") = arg2;
    register long a7 asm("a7") = ext;
    register long a6 asm("a6") = fid;
    asm volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a6), "r"(a7) : "memory");
    return a0;
}

volatile uint32_t counter = 0;
volatile uint32_t first = 1;

int main(void) {
    if (first) {
        first = 0;
        // Start secondary harts
        for (int i = 1; i < NUM_CORES; i++) {
            sbi_ecall(0x48534D, SBI_HART_START, i, (long) _entry, 0);
        }
    }

    for (int i = 0; i < 1000; i++) {
        // Atomic add 1
        __atomic_fetch_add(&counter, 1, __ATOMIC_RELAXED);
    }

    // Optional: spin forever
    while (1)
        ;
}
