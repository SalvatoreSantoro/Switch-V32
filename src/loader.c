#include "loader.h"
#include "cpu.h"
#include "memory.h"
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

enum {
    LD_OK = 0,
    LD_INVALID_ELF
};

struct Elf_File {
    size_t data_sz;
    size_t prog_size;
    size_t sect_size;
    uint8_t* data;
    Elf32_Phdr* programs;
    Elf32_Shdr* sections;
    Elf32_Ehdr* header;
};

#define VALIDATE(cond)             \
    do {                           \
        if (!(cond))               \
            return LD_INVALID_ELF; \
    } while (0)

static int _ld_validate_elf(Elf_File* elf)
{
    VALIDATE(elf->header->e_ident[EI_MAG0] == ELFMAG0);
    VALIDATE(elf->header->e_ident[EI_MAG1] == ELFMAG1);
    VALIDATE(elf->header->e_ident[EI_MAG2] == ELFMAG2);
    VALIDATE(elf->header->e_ident[EI_MAG3] == ELFMAG3);
    VALIDATE(elf->header->e_ident[EI_CLASS] == ELFCLASS32);
    VALIDATE(elf->header->e_ident[EI_CLASS] == ELFDATA2LSB);
    VALIDATE(elf->header->e_ident[EI_VERSION] == EV_CURRENT);
    VALIDATE(elf->header->e_ident[EI_OSABI] == ELFOSABI_SYSV);
    VALIDATE(elf->header->e_ident[EI_ABIVERSION] == 0);
    VALIDATE(elf->header->e_type == ET_EXEC);
    VALIDATE(elf->header->e_machine == EM_RISCV);
    VALIDATE(elf->header->e_entry != 0);
    VALIDATE(elf->header->e_phoff != 0);
    VALIDATE(elf->header->e_shoff != 0);

    return LD_OK;
}

static void _ld_elf_seg(Elf_File* elf)
{
    size_t fsz;
    uint32_t foff;
    uint32_t memsz;
    uint32_t addr;

    printf("memory: %p\n", __vmem.m);
    for (int i = 0; i < elf->header->e_phnum; i++) {
        if (elf->programs[i].p_type != PT_LOAD)
            continue;

        fsz = elf->programs[i].p_filesz;
        memsz = elf->programs[i].p_memsz;
        foff = elf->programs[i].p_offset;
        addr = elf->programs[i].p_vaddr;

        mem_wb_ptr_s(addr, (elf->data + foff), memsz);
        // for bss section
        // memory is currently static so should already be to 0, but just in case...
        if (fsz < memsz)
            mem_wb_s((addr + foff), 0, (memsz - fsz));

        // sleep(10);
        printf("Loaded Segment number %d, with addr %x and size %d in %p\n", i + 1, addr, memsz, __vmem.m + addr);
        // printf("%x\n", *(__vmem.m + addr));
    }
}

Elf_File* ld_elf(const char* file_name, VCore* core)
{
    int fd;
    struct stat fst;
    Elf_File* elf = NULL;

    if ((fd = open(file_name, O_RDONLY)) == -1)
        return NULL;

    if (fstat(fd, &fst) == -1)
        goto close;

    if ((elf = calloc(1, sizeof(Elf_File))) == NULL)
        goto close;

    if ((elf->data = mmap(NULL, fst.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) == NULL)
        goto free;

    elf->data_sz = fst.st_size;
    elf->header = (Elf32_Ehdr*)elf->data;

    if (_ld_validate_elf(elf) == LD_OK) {
        elf->programs = (Elf32_Phdr*)(elf->data + elf->header->e_phoff);
        elf->sections = (Elf32_Shdr*)(elf->data + elf->header->e_shoff);
        elf->prog_size = elf->header->e_phentsize * elf->header->e_phnum;
        elf->sect_size = elf->header->e_shentsize * elf->header->e_shnum;
        _ld_elf_seg(elf);
        // load entry point
        core->regs[PC] = elf->header->e_entry;
        goto close;
    }

    // if not LD_OK invalid file format
    errno = EINVAL;

free:
    free(elf);
    elf = NULL;
close:
    close(fd);
    return elf;
}

void ld_destroy_elf(Elf_File* elf)
{
    munmap(elf->data, elf->data_sz);
    free(elf);
}
