#include "system.h"
#include "cpu.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#define CLOSE 57
#define LSEEK 62
#define READ 63
#define WRITE 64
#define FSTAT 80
#define EXIT 93
#define GETTIMEOFDAY 169
#define BRK 214
#define CLOCK_GETTIME 403
#define OPEN 1024

void system_call(VCore* core)
{

    switch (core->regs[A7]) {
    case CLOSE:
        printf("CLOSE");
        core->regs[A0] = close(core->regs[A0]);
        break;
    case LSEEK:
        printf("LSEEK");
        core->regs[A0] = lseek(core->regs[A0], core->regs[A1], core->regs[A2]);
    case READ:
        printf("READ");
        core->regs[A0] = read(core->regs[A0], (void*)core->regs[A1], core->regs[A2]);
        break;
    case WRITE:
        printf("WRITE");
        core->regs[A0] = write(core->regs[A0], (void*)core->regs[A1], core->regs[A2]);
    case FSTAT:
        printf("FSTAT");
        core->regs[A0] = fstat(core->regs[A0], (struct stat*)core->regs[A1]);
    case GETTIMEOFDAY:
        printf("GETTIMEOFDAY");
        core->regs[A0] = gettimeofday((struct timeval*)core->regs[A0], (void*)core->regs[A1]);
    case EXIT:
        // TODO: IMPROVE MANAGMENT
        printf("EXIT %d\n", core->regs[A0]);
        exit(core->regs[A0]);
    case OPEN:
        core->regs[A0] = open((const char*)core->regs[A0], core->regs[A1]);
        break;
    }
}
