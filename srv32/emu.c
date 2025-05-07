#include "emu.h"
#include "../common/sdl_syscalls.h"
#include "cpu.h"
#include "macros.h"
#include "memory.h"
#include "sdl.h"
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define EMU_CRASH(str)                                                                                                 \
    do {                                                                                                               \
        fprintf(stderr, "The system EMULATION crashed: %s\n", str);                                                    \
        exit(EXIT_FAILURE);                                                                                            \
    } while (0)


// System calls needed from Newlib

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

// supposing 1024 maximum file descriptors
// 0-511 for this program 512-1023 for the elf

#define ELF_FDS_BASELINE 512
#define EMU_FD

// need to "translate" x86-64 struct stat to rv32
// referring to Newlib implementation of "sys/stat.h"
struct EMU_timespec {
    int64_t tv_sec;  /* seconds */
    int32_t tv_nsec; /* and nanoseconds */
};

// they're the same should optimize this...
struct EMU_timeval {
    int64_t tv_sec;
    int32_t tv_usec;
};

struct EMU_stat {
    int16_t st_dev;
    uint16_t st_ino;
    uint32_t st_mode;
    uint16_t st_nlink;
    uint16_t st_uid;
    uint16_t st_gid;
    int16_t st_rdev;
    int32_t st_size;
    struct EMU_timespec st_atim;
    struct EMU_timespec st_mtim;
    struct EMU_timespec st_ctim;
    int32_t st_blksize;
    int32_t st_blocks;
    int32_t st_spare4[2];
};

static void _emu_copy_timespec(struct EMU_timespec *ets, struct timespec *ts) {
    ets->tv_sec = ts->tv_sec;
    ets->tv_nsec = ts->tv_nsec;
}

static void _emu_copy_timeval(struct EMU_timeval *etv, struct timeval *tv) {
    etv->tv_sec = tv->tv_sec;
    etv->tv_usec = tv->tv_usec;
}

static void _emu_copy_stat(struct EMU_stat *es, struct stat *fs) {
    es->st_dev = fs->st_dev;
    es->st_ino = fs->st_ino;
    es->st_mode = fs->st_mode;
    es->st_nlink = fs->st_nlink;
    es->st_uid = fs->st_uid;
    es->st_gid = fs->st_gid;
    es->st_rdev = fs->st_rdev;
    es->st_size = fs->st_size;
    _emu_copy_timespec(&es->st_atim, &fs->st_atim);
    _emu_copy_timespec(&es->st_mtim, &fs->st_mtim);
    _emu_copy_timespec(&es->st_ctim, &fs->st_ctim);
    es->st_blksize = fs->st_blksize;
    es->st_blocks = fs->st_blocks;
}

void emu_std(const char *stdin_name, const char *stdout_name, const char *stderr_name) {
    int ret;
    int fd;

    if (stdin_name == NULL)
        ret = dup2(STDIN_FILENO, ELF_FDS_BASELINE);
    else {
        fd = open(stdin_name, O_RDONLY);
        ret = dup2(fd, ELF_FDS_BASELINE);
        close(fd);
    }
    if (ret == -1)
        EMU_CRASH("CAN'T SET STDIN");

    if (stdout_name == NULL)
        dup2(STDOUT_FILENO, ELF_FDS_BASELINE + 1);
    else {
        fd = open(stdout_name, O_WRONLY | O_TRUNC | O_CREAT, 0644);
        ret = dup2(fd, ELF_FDS_BASELINE + 1);
        close(fd);
    }
    if (ret == -1)
        EMU_CRASH("CAN'T SET STDOUT");

    if (stderr_name == NULL)
        dup2(STDERR_FILENO, ELF_FDS_BASELINE + 2);
    else {
        fd = open(stderr_name, O_WRONLY | O_TRUNC | O_CREAT, 0644);
        ret = dup2(fd, ELF_FDS_BASELINE + 2);
        close(fd);
    }
    if (ret == -1)
        EMU_CRASH("CAN'T SET STDERR");
}

void emu_args(int elf_argc, ...) {
    const char *str;
    size_t str_size = 0;
    size_t str_size_inc = 0;
    va_list ap;

    // the layout is |STACK_BASE|ARGC|ARGV[0]|ARGV[1]|...|ARGV[ARGC-1]|NULL|STR0|STR1|...|STR_ARGC-1|
    mem_ww(STACK_BASE, elf_argc);

    uint32_t arg_str_start = STACK_BASE + 4 + ((elf_argc + 1) * 4);
    uint32_t arg_ptr_start = STACK_BASE + 4;

    va_start(ap, elf_argc);
    for (int i = 0; i < elf_argc; i++) {
        str = va_arg(ap, const char *);
        str_size = strlen(str) + 1; // keep in mind the '\0'
        mem_wb_ptr_s(arg_str_start + str_size_inc, str, str_size);
        mem_ww(arg_ptr_start + (i * 4), arg_str_start + str_size_inc);
        str_size_inc += str_size;
    }

    mem_ww(arg_ptr_start + (elf_argc * 4), 0);
    va_end(ap);
}

void emu_system_call(VCore *core) {
    int fd;
    struct stat fs;
    struct timespec ts;
    struct timeval tv;
    uintptr_t tmp_addr;
    uintptr_t tmp_addr2;
    switch (core->regs[A7]) {
    case CLOSE:
        fd = ELF_FDS_BASELINE + core->regs[A0];
        LOG_DE("CLOSE", fd);
        core->regs[A0] = close(fd);
        break;
    case LSEEK:
        fd = ELF_FDS_BASELINE + core->regs[A0];
        LOG_DE("LSEEK", fd);
        core->regs[A0] = lseek(fd, (off_t) core->regs[A1], (int) core->regs[A2]);
        break;
    case READ:
        tmp_addr = core->regs[A1];
        fd = ELF_FDS_BASELINE + core->regs[A0];
        LOG_DE("READ", fd);
        core->regs[A0] = read(fd, (void *) (__vmem.m + tmp_addr), core->regs[A2]);
        break;
    case WRITE:
        tmp_addr = core->regs[A1];
        fd = ELF_FDS_BASELINE + core->regs[A0];
        LOG_DE("WRITE", fd);
        core->regs[A0] = write(fd, (void *) (__vmem.m + tmp_addr), core->regs[A2]);
        break;
    case FSTAT:
        fd = ELF_FDS_BASELINE + core->regs[A0];
        LOG_DE("FSTAT", fd);
        tmp_addr = core->regs[A1];
        core->regs[A0] = fstat(fd, &fs);
        _emu_copy_stat((struct EMU_stat *) (__vmem.m + tmp_addr), &fs);
        break;
    case GETTIMEOFDAY:
        LOG_DE("GETTIMEOFDAY", 0);
        tmp_addr = core->regs[A0];
        tmp_addr2 = core->regs[A1];
        core->regs[A0] = gettimeofday(&tv, (void *) tmp_addr2);
        _emu_copy_timeval((struct EMU_timeval *) (__vmem.m + tmp_addr), &tv);
        break;
    case EXIT:
        LOG_DE("EXIT", core->regs[A0]);
        tmp_addr = (uintptr_t) (__vmem.m + core->elf_errno);
        if (tmp_addr != 0)
            fprintf(stderr, "Last application ERRNO: %d %s\n", *((int *) tmp_addr), strerror(*(int *) tmp_addr));
        exit(core->regs[A0]);
        break;
    case CLOCK_GETTIME:
        tmp_addr = core->regs[A1];
        core->regs[A0] = clock_gettime(core->regs[A0], &ts);
        _emu_copy_timespec((struct EMU_timespec *) (__vmem.m + tmp_addr), &ts);
        LOG_DE("CLOCK_GETTIME", core->regs[A0]);
        break;
    case OPEN:
        tmp_addr = core->regs[A0];
        LOG_STR("OPEN", (const char *) (__vmem.m + tmp_addr));
        fd = open((const char *) (__vmem.m + tmp_addr), core->regs[A1]);
        core->regs[A0] = fd;
        if (fd >= ELF_FDS_BASELINE) {
            fprintf(stderr, "THE ELF FILE OPENED TOO MANY FDs\n");
            exit(EXIT_FAILURE);
        }
        dup2(fd, ELF_FDS_BASELINE + fd);
        close(fd);
        break;
    case BRK:
        LOG_EX("BRK", core->regs[A0]);
        tmp_addr = (uintptr_t) __vmem.m + core->regs[A0];
        if ((tmp_addr) >= BRK_LIMIT) {
            fprintf(stderr, "THE PROCESS TRIED ALLOCATING TOO MUCH HEAP MEMORY\n");
            core->regs[A0] = -1;
            break;
        }
        if (core->regs[A0] > 0)
            core->elf_brk = core->regs[A0];
        core->regs[A0] = core->elf_brk;
        break;
        // SDL
    case SDL_INIT:
        tmp_addr = (uintptr_t) __vmem.m + core->regs[A0];
        sdl_init((const char *) tmp_addr, core->regs[A1], core->regs[A2]);
        break;
    case SDL_WRITE_PALETTE:
        tmp_addr = (uintptr_t) __vmem.m + core->regs[A0];
        sdl_write_palette((uint32_t *) tmp_addr, core->regs[A1]);
        break;
    case SDL_WRITE_FB:
        tmp_addr = (uintptr_t) __vmem.m + core->regs[A0];
        sdl_write_fb((const uint8_t *) tmp_addr);
        break;
    case SDL_SHUTDOWN:
        sdl_shutdown();
        break;
    default:
        fprintf(stderr, "UNSUPPORTED SYSCALL %d at %x\n", core->regs[A7], core->pc);
        core->regs[A0] = 0;
        exit(EXIT_FAILURE);
    }
}
