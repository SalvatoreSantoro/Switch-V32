#include "emu.h"
#include "args.h"
#include "cpu.h"
#include "macros.h"
#include "memory.h"
#include "sdl.h"
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
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

// need to "translate" x86-64 struct stat to rv32
// referring to Newlib implementation of "sys/stat.h"
struct EMU_timespec {
    int64_t tv_sec;  /* seconds */
    int32_t tv_nsec; /* and nanoseconds */
};

// they're the same should get rid of this...
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

void emu_std() {
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
        EMU_CRASH("CAN'T SET STDIN");

    if (ctx.elf_stdout == NULL)
        dup2(STDOUT_FILENO, ELF_FDS_BASELINE + 1);
    else {
        fd = open(ctx.elf_stdout, O_WRONLY | O_TRUNC | O_CREAT, 0644);
        ret = dup2(fd, ELF_FDS_BASELINE + 1);
        close(fd);
    }
    if (ret == -1)
        EMU_CRASH("CAN'T SET STDOUT");

    if (ctx.elf_stderr == NULL)
        dup2(STDERR_FILENO, ELF_FDS_BASELINE + 2);
    else {
        fd = open(ctx.elf_stderr, O_WRONLY | O_TRUNC | O_CREAT, 0644);
        ret = dup2(fd, ELF_FDS_BASELINE + 2);
        close(fd);
    }
    if (ret == -1)
        EMU_CRASH("CAN'T SET STDERR");
}

void emu_args() {
    const char *str;
    char *tmp_str;
    char *token;
    unsigned int elf_argc = 0;
    size_t str_size = 0;
    size_t str_size_inc = 0;
    size_t tmp_str_sz;

    tmp_str_sz = strlen(ctx.elf_args);
    if (tmp_str_sz >= PAGE_SIZE)
        EMU_CRASH("ELF ARGUMENT TOO BIG");

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
        str_size = strlen(str) + 1; // keep in mind the '\0'
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
        fd = ELF_FDS_BASELINE + core->regs[A0];
        LOG_DE("READ", fd);
        core->regs[A0] = read(fd, MAP_ADDR(core->regs[A1]), core->regs[A2]);
        break;

    case WRITE:
        fd = ELF_FDS_BASELINE + core->regs[A0];
        LOG_DE("WRITE", fd);
        core->regs[A0] = write(fd, MAP_ADDR(core->regs[A1]), core->regs[A2]);
        break;

    case FSTAT:
        fd = ELF_FDS_BASELINE + core->regs[A1];
        LOG_DE("FSTAT", fd);
        core->regs[A0] = fstat(fd, &fs);
        _emu_copy_stat(MAP_ADDR(core->regs[A1]), &fs);
        break;

    case GETTIMEOFDAY:
        LOG_DE("GETTIMEOFDAY", 0);
        core->regs[A0] = gettimeofday(&tv, MAP_ADDR(core->regs[A1]));
        _emu_copy_timeval(MAP_ADDR(core->regs[A0]), &tv);
        break;

    case EXIT:
        LOG_DE("EXIT", core->regs[A0]);
        tmp_addr = MAP_ADDR(core->elf_errno);
        if (tmp_addr != 0)
            fprintf(stderr, "Last application ERRNO: %d %s\n", *((int *) tmp_addr), strerror(*(int *) tmp_addr));
        exit(core->regs[A0]);
        break;

    case CLOCK_GETTIME:
        core->regs[A0] = clock_gettime(core->regs[A0], &ts);
        LOG_DE("CLOCK_GETTIME", core->regs[A0]);
        _emu_copy_timespec(MAP_ADDR(core->regs[A1]), &ts);
        break;

    case OPEN:
        LOG_STR("OPEN", MAP_ADDR(core->regs[A0]));
        fd = open(MAP_ADDR(core->regs[A0]), core->regs[A1]);
        core->regs[A0] = fd;
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
            core->regs[A0] = -1;
            break;
        }
        if (core->regs[A0] > 0)
            core->elf_brk = core->regs[A0];
        core->regs[A0] = core->elf_brk;
        break;

        // SDL
    case SDL_INIT:
        sdl_init(MAP_ADDR(core->regs[A0]), core->regs[A1], core->regs[A2], core->regs[A3]);
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
