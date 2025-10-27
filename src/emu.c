#define _POSIX_C_SOURCE 200809L
#include "emu.h"
#include "args.h"
#include "cpu.h"
#include "macros.h"
#include "memory.h"
#include "sdl.h"
#include <fcntl.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

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

// need to "translate" x86-64 struct stat to rv32
// referring to Newlib implementation of "sys/stat.h"
struct EMU_timespec {
    int64_t tv_sec;  /* seconds */
    int32_t tv_nsec; /* and nanoseconds */
};

struct EMU_timeval {
    int64_t tv_sec;
    int32_t tv_usec;
};

// we're truncating basically every field when emulating fstat
// so that system call is basically unreliable
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

static void emu_copy_timespec(struct EMU_timespec *ets, struct timespec *ts) {

    int64_t sec = ts->tv_sec;
    int64_t nsec = ts->tv_nsec;

    // Handle nsec overflow / underflow beyond 32-bit
    if (nsec > INT32_MAX || nsec < INT32_MIN) {
        int64_t extra_sec = nsec / 1000000000LL; // full seconds from nsec
        nsec -= extra_sec * 1000000000LL;
        sec += extra_sec;
    }

    // Handle negative nsec that is still within int32 range
    if (nsec < 0) {
        sec -= 1;
        nsec += 1000000000LL;
    }

    ets->tv_sec = sec;
    ets->tv_nsec = (int32_t) nsec;
}

static void emu_copy_timeval(struct EMU_timeval *etv, struct timeval *tv) {
    int64_t sec = tv->tv_sec;
    int64_t usec = tv->tv_usec;

    // Normalize microseconds → seconds
    if (usec > INT32_MAX || usec < INT32_MIN) {
        int64_t extra_sec = usec / 1000000LL; // full seconds from usec
        usec -= extra_sec * 1000000LL;
        sec += extra_sec;
    }

    // Handle negative usec that is still within int32 range
    if (usec < 0) {
        sec -= 1;
        usec += 1000000LL;
    }
    etv->tv_sec = sec;
    etv->tv_usec = (int32_t) usec;
}

static void emu_copy_stat(struct EMU_stat *es, struct stat *fs) {
    es->st_dev = (int16_t) fs->st_dev;
    es->st_ino = (uint16_t) fs->st_ino;
    es->st_mode = (uint32_t) fs->st_mode;
    es->st_nlink = (uint16_t) fs->st_nlink;
    es->st_uid = (uint16_t) fs->st_uid;
    es->st_gid = (uint16_t) fs->st_gid;
    es->st_rdev = (int16_t) fs->st_rdev;

    if (fs->st_size > INT32_MAX || fs->st_size < INT32_MIN)
        es->st_size = INT32_MAX;
    else
        es->st_size = (int32_t) fs->st_size;

    emu_copy_timespec(&es->st_atim, &fs->st_atim);
    emu_copy_timespec(&es->st_mtim, &fs->st_mtim);
    emu_copy_timespec(&es->st_ctim, &fs->st_ctim);

    es->st_blksize = (int32_t) fs->st_blksize;
    es->st_blocks = (int32_t) fs->st_blocks;
}

void emu_std(void) {
    int ret;
    int fd;

    if (ctx.elf_stdin == NULL)
        ret = dup2(STDIN_FILENO, ELF_FDS_BASELINE);
    else {
        fd = open(ctx.elf_stdin, O_RDONLY);
        ret = dup2(fd, ELF_FDS_BASELINE);
        close(fd);
    }
    if (ret == -1)
        SV32_CRASH("CAN'T SET STDIN");

    if (ctx.elf_stdout == NULL)
        dup2(STDOUT_FILENO, ELF_FDS_BASELINE + 1);
    else {
        fd = open(ctx.elf_stdout, O_WRONLY | O_TRUNC | O_CREAT, 0644);
        ret = dup2(fd, ELF_FDS_BASELINE + 1);
        close(fd);
    }
    if (ret == -1)
        SV32_CRASH("CAN'T SET STDOUT");

    if (ctx.elf_stderr == NULL)
        dup2(STDERR_FILENO, ELF_FDS_BASELINE + 2);
    else {
        fd = open(ctx.elf_stderr, O_WRONLY | O_TRUNC | O_CREAT, 0644);
        ret = dup2(fd, ELF_FDS_BASELINE + 2);
        close(fd);
    }
    if (ret == -1)
        SV32_CRASH("CAN'T SET STDERR");
}

void emu_args(void) {
    const char *str;
    char *tmp_str;
    char *token;
    unsigned int elf_argc = 0;
    uint32_t str_size = 0;
    uint32_t tmp_str_sz;
    uint32_t str_size_inc = 0;

    tmp_str_sz = (uint32_t) strlen(ctx.elf_args);
    if (tmp_str_sz >= PAGE_SIZE)
        SV32_CRASH("ELF ARGUMENT TOO BIG");

    // create temp string for tokenizer
    tmp_str = malloc(tmp_str_sz + 1);
    memcpy(tmp_str, ctx.elf_args, tmp_str_sz);
    tmp_str[tmp_str_sz] = '\0';

    // tokenize to count argc
    token = strtok(tmp_str, " ");
    while (token != NULL) {
        elf_argc += 1;
        token = strtok(NULL, " ");
    }

    // the layout is |STACK_BASE|ARGC|ARGV[0]|ARGV[1]|...|ARGV[ARGC-1]|NULL|STR0|STR1|...|STR_ARGC-1|
    mem_ww(STACK_BASE, elf_argc);

    uint32_t arg_str_start = STACK_BASE + 4 + ((elf_argc + 1) * 4);
    uint32_t arg_ptr_start = STACK_BASE + 4;

    str = tmp_str;
    for (unsigned int i = 0; i < elf_argc; i++) {
        str_size = (uint32_t) strlen(str) + 1; // keep in mind the '\0'
        mem_wb_ptr_s(arg_str_start + str_size_inc, str, str_size);
        mem_ww(arg_ptr_start + (i * 4), arg_str_start + str_size_inc);
        str_size_inc += str_size;
        str += str_size;
    }

    mem_ww(arg_ptr_start + (elf_argc * 4), 0);
    free(tmp_str);
}

void emu_system_call(VCore *core) {
    struct stat fs;
    struct timespec ts;
    struct timeval tv;
    int fd;
    void *tmp_addr;

    switch (core->regs[A7]) {

    case CLOSE:
        fd = (int) core->regs[A0];
        if (fd < 0) {
            core->regs[A0] = (uint32_t) -1;
        } else {
            fd += ELF_FDS_BASELINE;
            core->regs[A0] = (uint32_t) close(fd);
        }
        LOG_DE("CLOSE", fd);
        break;

    case LSEEK:
        fd = (int) core->regs[A0];
        if (fd < 0) {
            core->regs[A0] = (uint32_t) -1;
        } else {
            fd += ELF_FDS_BASELINE;
            core->regs[A0] = (uint32_t) lseek(fd, (off_t) core->regs[A1], (int) core->regs[A2]);
        }
        LOG_DE("LSEEK", fd);
        break;

    case READ:
        fd = (int) core->regs[A0];
        if (fd < 0) {
            core->regs[A0] = (uint32_t) -1;
        } else {
            fd += ELF_FDS_BASELINE;
            core->regs[A0] = (uint32_t) read(fd, MAP_ADDR(core->regs[A1]), core->regs[A2]);
        }
        LOG_DE("READ", fd);
        break;

    case WRITE:
        fd = (int) core->regs[A0];
        if (fd < 0) {
            core->regs[A0] = (uint32_t) -1;
        } else {
            fd += ELF_FDS_BASELINE;
            core->regs[A0] = (uint32_t) write(fd, MAP_ADDR(core->regs[A1]), core->regs[A2]);
        }
        LOG_DE("WRITE", fd);
        break;

    case FSTAT:
        fd = (int) core->regs[A0];
        if (fd < 0) {
            core->regs[A0] = (uint32_t) -1;
        } else {
            fd += ELF_FDS_BASELINE;
            core->regs[A0] = (uint32_t) fstat(fd, &fs);
            emu_copy_stat(MAP_ADDR(core->regs[A1]), &fs);
        }
        LOG_DE("FSTAT", fd);
        break;

    case GETTIMEOFDAY:
        LOG_DE("GETTIMEOFDAY", 0);
        core->regs[A0] = (uint32_t) gettimeofday(&tv, MAP_ADDR(core->regs[A1]));
        emu_copy_timeval(MAP_ADDR(core->regs[A0]), &tv);
        break;

    case EXIT:
        LOG_DE("EXIT", core->regs[A0]);
        tmp_addr = MAP_ADDR(core->elf_errno);
        if (tmp_addr != 0)
            fprintf(stderr, "Last application ERRNO: %d %s\n", *((int *) tmp_addr), strerror(*(int *) tmp_addr));
        exit((int) core->regs[A0]);
        break;

    case CLOCK_GETTIME:
        core->regs[A0] = (uint32_t) clock_gettime((clockid_t) core->regs[A0], &ts);
        LOG_DE("CLOCK_GETTIME", core->regs[A0]);
        emu_copy_timespec(MAP_ADDR(core->regs[A1]), &ts);
        break;

    case OPEN:
        LOG_STR("OPEN", MAP_ADDR(core->regs[A0]));
        fd = open(MAP_ADDR(core->regs[A0]), (int) core->regs[A1]);
        core->regs[A0] = (uint32_t) fd;
        // error
        if (fd < 0)
            return;
        if (fd >= ELF_FDS_BASELINE) {
            fprintf(stderr, "THE ELF FILE OPENED TOO MANY FDs\n");
            exit(EXIT_FAILURE);
        }
        dup2(fd, ELF_FDS_BASELINE + fd);
        close(fd);
        break;

    case BRK:
        LOG_EX("BRK", MAP_ADDR(core->regs[A0]));
        if ((MAP_ADDR(core->regs[A0]) >= MAP_ADDR(BRK_LIMIT))) {
            fprintf(stderr, "THE PROCESS TRIED ALLOCATING TOO MUCH HEAP MEMORY\n");
            core->regs[A0] = (uint32_t) -1;
            break;
        }
        if (core->regs[A0] > 0)
            core->elf_brk = core->regs[A0];
        core->regs[A0] = core->elf_brk;
        break;

        // SDL
    case SDL_INIT:
        sdl_init(MAP_ADDR(core->regs[A0]), (int) core->regs[A1], (int) core->regs[A2], core->regs[A3]);
        break;

    case SDL_WRITE_PALETTE:
        sdl_write_palette(MAP_ADDR(core->regs[A0]), core->regs[A1]);
        break;

    case SDL_WRITE_FB:
        sdl_write_fb(MAP_ADDR(core->regs[A0]));
        break;

    case SDL_SHUTDOWN:
        sdl_shutdown();
        break;

    case SDL_PULL_EVENTS:
        core->regs[A0] = sdl_pull_events(MAP_ADDR(core->regs[A0]), MAP_ADDR(core->regs[A1]), core->regs[A2]);
        break;

    default:
        fprintf(stderr, "UNSUPPORTED SYSCALL %d at %x\n", core->regs[A7], core->pc);
        core->regs[A0] = 0;
        exit(EXIT_FAILURE);
    }
}
