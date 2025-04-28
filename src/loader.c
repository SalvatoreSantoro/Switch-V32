#include "loader.h"
#include "cpu.h"
#include "memory.h"
#include <assert.h>
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
    size_t symb_size;
    uint8_t* data;
    Elf32_Phdr* programs;
    Elf32_Shdr* sections;
    Elf32_Ehdr* header;
    Elf32_Sym* symbols;
    const char* symbols_names;
    const char* sections_names;
};

#define VALIDATE(cond)             \
    do {                           \
        if (!(cond))               \
            return LD_INVALID_ELF; \
    } while (0)

static int _ld_elf_validate(Elf_File* elf)
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
    VALIDATE(elf->header->e_shstrndx != SHN_UNDEF);

    return LD_OK;
}

static Elf32_Shdr* _ld_elf_getsect(Elf_File* elf, const char* sect_name)
{
    Elf32_Shdr* sect;
    for (int i = 0; i < elf->header->e_shnum; i++) {
        sect = &elf->sections[i];
        // the section name is offset inside the string of names
        if (strcmp(sect_name, elf->sections_names + sect->sh_name) == 0)
            return sect;
    }
    return NULL;
}

static Elf32_Sym* _ld_elf_getsym(Elf_File* elf, const char* sym_name)
{
    Elf32_Sym* sym;
    printf("SIZE %ld\n", elf->symb_size);
    for (size_t i = 0; i < elf->symb_size; i++) {
        sym = &elf->symbols[i];
        if (strcmp(sym_name, elf->symbols_names + sym->st_name) == 0)
            return sym;
    }
    return NULL;
}

// Citing elf(5) man page
// If the index of section name string table section is larger
// than or equal to SHN_LORESERVE (0xff00), this member holds
// SHN_XINDEX (0xffff) and the real index of the section name
// string table section is held in the sh_link member of the
// initial entry in section header table.  Otherwise, the
// sh_link member of the initial entry in section header table
// contains the value zero.

static void _ld_elf_section_names(Elf_File* elf)
{
    Elf32_Shdr shstrtab;
    uint32_t idx = elf->header->e_shstrndx;

    if (idx >= SHN_LORESERVE)
        idx = elf->sections[0].sh_link;

    shstrtab = elf->sections[idx];

    elf->sections_names = (const char*)(elf->data + shstrtab.sh_offset);
}

static void _ld_elf_symbols(Elf_File* elf)
{
    Elf32_Shdr* sect;

    // load symbols table
    sect = _ld_elf_getsect(elf, ".symtab");
    assert(((sect != NULL) && (sect->sh_type == SHT_SYMTAB)) && "CAN'T FIND .symtab");
    elf->symb_size = sect->sh_size / sect->sh_entsize;
    elf->symbols = (Elf32_Sym*)(elf->data + sect->sh_offset);

    // load symbols names
    sect = _ld_elf_getsect(elf, ".strtab");
    assert(((sect != NULL) && (sect->sh_type == SHT_STRTAB)) && "CAN'T FIND .strtab");
    elf->symbols_names = (const char*)(elf->data + sect->sh_offset);
}

/* static void _ld_find_section */

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
        if (fsz < memsz) {
            mem_wb_s((addr + fsz), 0, (memsz - fsz));
        }

        // sleep(10);
        printf("Loaded Segment number %d, with addr %x and size %d in %p\n", i + 1, addr, memsz, __vmem.m + addr);
        // printf("%x\n", *(__vmem.m + addr));
    }
}

Elf_File* ld_elf(const char* file_name, VCore* core)
{
    int fd;
    struct stat fst;
    Elf32_Sym* sym;
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

    if (_ld_elf_validate(elf) == LD_OK) {
        elf->programs = (Elf32_Phdr*)(elf->data + elf->header->e_phoff);
        elf->sections = (Elf32_Shdr*)(elf->data + elf->header->e_shoff);
        elf->prog_size = elf->header->e_phentsize * elf->header->e_phnum;
        elf->sect_size = elf->header->e_shentsize * elf->header->e_shnum;
        // init section names string
        _ld_elf_section_names(elf);
        // init symbols string
        _ld_elf_symbols(elf);

        // load segments
        _ld_elf_seg(elf);
        // load entry point
        core->regs[PC] = elf->header->e_entry;
        // load stack base
        core->regs[SP] = STACK_BASE;
        // load GP
        sym = _ld_elf_getsym(elf, "__global_pointer$");
        assert(sym && "CAN'T FIND SYMBOL __global_pointer$");
        core->regs[GP] = sym->st_value;
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
