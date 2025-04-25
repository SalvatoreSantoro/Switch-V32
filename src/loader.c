#include "loader.h"
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

#define ELF_HDR_SIZE 52

enum {
    LD_OK = 0,
    LD_INVALID_ELF
};

#define CHECK_MAGIC(e)                              \
    ((e->header->e_ident[EI_MAG0] == ELFMAG0)       \
        && (e->header->e_ident[EI_MAG1] == ELFMAG1) \
        && (e->header->e_ident[EI_MAG2] == ELFMAG2) \
        && (e->header->e_ident[EI_MAG3] == ELFMAG3))

#define CHECK_CLASS(e) \
    (e->header->e_ident[EI_CLASS] == ELFCLASS32)

#define CHECK_DATA(e) \
    (e->header->e_ident[EI_CLASS] == ELFDATA2LSB)

#define CHECK_ELF_VER(e) \
    (e->header->e_ident[EI_VERSION] == EV_CURRENT)

#define CHECK_ABI(e) \
    (e->header->e_ident[EI_OSABI] == ELFOSABI_SYSV)

#define CHECK_ABIVER(e) \
    (e->header->e_ident[EI_ABIVERSION] == 0)

#define CHECK_TYPE(e) \
    (e->header->e_type == ET_EXEC)

#define CHECK_MACHINE(e) \
    (e->header->e_machine == EM_RISCV)

#define CHECK_ENTRY(e) \
    (e->header->e_entry != 0)

#define CHECK_PRG_HDR(e) \
    (e->header->e_phoff != 0)

#define CHECK_SEC_HDR(e) \
    (e->header->e_shoff != 0)

struct Elf_File {
    size_t data_sz;
    size_t prog_size;
    size_t sect_size;
    uint8_t* data;
    Elf32_Phdr* programs;
    Elf32_Shdr* sections;
    Elf32_Ehdr* header;
};

static int _ld_validate_elf(Elf_File* elf)
{
    if (!CHECK_MAGIC(elf))
        return LD_INVALID_ELF;

    if (!CHECK_CLASS(elf))
        return LD_INVALID_ELF;

    if (!CHECK_DATA(elf))
        return LD_INVALID_ELF;

    if (!CHECK_ELF_VER(elf))
        return LD_INVALID_ELF;

    if (!CHECK_ABI(elf))
        return LD_INVALID_ELF;

    if (!CHECK_ABIVER(elf))
        return LD_INVALID_ELF;

    if (!CHECK_TYPE(elf))
        return LD_INVALID_ELF;

    if (!CHECK_MACHINE(elf))
        return LD_INVALID_ELF;

    if (!CHECK_ENTRY(elf))
        return LD_INVALID_ELF;

    if (!CHECK_PRG_HDR(elf))
        return LD_INVALID_ELF;

    if (!CHECK_SEC_HDR(elf))
        return LD_INVALID_ELF;

    // CHECK E_FLAGS

    return LD_OK;
}

static void _ld_elf(Elf_File* elf)
{
    size_t fsz;
    uint32_t foff;
    uint32_t memsz;
    uint32_t addr;


    printf("memory: %p\n", __mem.m);
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

        //sleep(10);
        printf("Loaded Segment number %d, with addr %x and size %d in %p\n", i + 1, addr, memsz, __mem.m + addr);
    }
}

Elf_File* ld_elf(const char* file_name)
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
        _ld_elf(elf);
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
