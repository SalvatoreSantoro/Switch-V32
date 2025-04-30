#include "system.h"
#include "cpu.h"
#include "memory.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define CLOSE         57
#define LSEEK         62
#define READ          63
#define WRITE         64
#define FSTAT         80
#define EXIT          93
#define GETTIMEOFDAY  169
#define BRK           214
#define CLOCK_GETTIME 403
#define OPEN          1024

void system_call(VCore *core) {
    uintptr_t tmp_addr;
    uintptr_t tmp_addr2;
    switch (core->regs[A7]) {
    case CLOSE:
        printf("CLOSE %d\n", core->regs[A0]);
        core->regs[A0] = close(core->regs[A0]);
        break;
    case LSEEK:
        printf("LSEEK %d\n", core->regs[A0]);
        core->regs[A0] = lseek(core->regs[A0], core->regs[A1], core->regs[A2]);
        break;
    case READ:
        tmp_addr = core->regs[A1];
        printf("READ %d \n", core->regs[A0]);
        core->regs[A0] = read(core->regs[A0], (void *) (__vmem.m + tmp_addr), core->regs[A2]);
        break;
    case WRITE:
        tmp_addr = core->regs[A1];
        printf("WRITE %x \n", core->regs[A0]);
        core->regs[A0] = write(core->regs[A0], (void *) (__vmem.m + tmp_addr), core->regs[A2]);
        break;
    case FSTAT:
        tmp_addr = core->regs[A1];
        printf("FSTAT %d\n", core->regs[A0]);
        core->regs[A0] = fstat(core->regs[A0], (struct stat *) tmp_addr);
        break;
    case GETTIMEOFDAY:
        tmp_addr = core->regs[A0];
        tmp_addr2 = core->regs[A1];
        printf("GETTIMEOFDAY\n");
        core->regs[A0] = gettimeofday((struct timeval *) tmp_addr, (void *) tmp_addr2);
        break;
    case EXIT:
        tmp_addr = core->elf_errno;
        printf("EXIT %d\n", core->regs[A0]);
        fprintf(stderr, "Last application ERRNO: %d, %s", *(int *) tmp_addr, strerror(*(int *) tmp_addr));
        exit(core->regs[A0]);
        break;
    case OPEN:
        tmp_addr = core->regs[A0];
        printf("OPEN %d\n", core->regs[A0]);
        core->regs[A0] = open((const char *) (__vmem.m + tmp_addr), core->regs[A1]);
        break;
    case BRK:
        printf("BRK %x\n", core->regs[A0]);
        if (core->regs[A0] > 0) {
            core->brk = core->regs[A0];
            core->regs[A0] = 0;
        } else
            core->regs[A0] = core->brk;
        break;
    }
}
