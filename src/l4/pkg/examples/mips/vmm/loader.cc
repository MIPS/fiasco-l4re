/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#include <sys/types.h>
#include <elf.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#include "debug.h"
#include "loader.h"
#include "mipsregs.h"

#undef LOADER_DEBUG

#define OUR_ELF_ENDIAN      ELFDATA2LSB
#define ELF_OUR_MACHINE     EM_MIPS

#define SYM_TEXT    1
#define SYM_DATA    2

#define ELF_SYMTAB      ".symtab"       /* symbol table */
#define ELF_TEXT        ".text"         /* code */
#define ELF_STRTAB      ".strtab"

#define IS_ELF(ehdr) ((ehdr).e_ident[EI_MAG0] == ELFMAG0 && \
                      (ehdr).e_ident[EI_MAG1] == ELFMAG1 && \
                      (ehdr).e_ident[EI_MAG2] == ELFMAG2 && \
                      (ehdr).e_ident[EI_MAG3] == ELFMAG3)



struct sym_ent
{
    l4_int8_t *name;               /* name of symbol entry */
    l4_uint8_t type;               /* type - either text or data for now */
    l4_addr_t  addr;               /* address of symbol */
};
typedef struct sym_ent Sym_ent;

struct sym_table
{
    Sym_ent    *list;              /* unordered */
    l4_uint32_t num;
};
typedef struct sym_table Sym_table;

#define ELF_VADDR_TO_OFFSET(base,addr,offset) ((base) + MIPS_KSEG0_TO_PHYS((addr)) - (offset))


bool
loader_elf_is_exec(l4_uint8_t * load, l4_uint32_t loadlen)
{
    Elf32_Ehdr *hdr = (Elf32_Ehdr *) load;

    if (loadlen >= sizeof *hdr &&
        IS_ELF(*hdr) &&
        hdr->e_ident[EI_CLASS] == ELFCLASS32 &&
        hdr->e_ident[EI_DATA] == OUR_ELF_ENDIAN &&
        hdr->e_type == ET_EXEC && hdr->e_machine == ELF_OUR_MACHINE)
        return true;

    return false;
}

/* prepare to run the ELF image
 */
int
loader_elf_load(l4_uint8_t * load, l4_uint32_t loadlen, l4_uint8_t *mem,
              l4_addr_t vaddr_offset, l4_addr_t *entrypoint)
{
    Elf32_Ehdr *hdr = (Elf32_Ehdr *) load;
    Elf32_Phdr *phdr;
    /*Elf32_Shdr *shdr; */
    int i;

    if (!loader_elf_is_exec(load, loadlen))       /* sanity check */
        return EINVAL;

    /* copy our headers to temp memory to let us copy over our image */
    i = sizeof *hdr + hdr->e_phoff + sizeof *phdr * hdr->e_phnum;
#ifdef LOADER_DEBUG
    karma_log(DEBUG, "sizeof *hdr=%d sizeof *phdr=%d phnum=%d len=%d\n",
           sizeof *hdr, sizeof *phdr, hdr->e_phnum, i);
#endif

    hdr = (Elf32_Ehdr *) malloc(i);

    if (hdr == NULL) {
        karma_log(ERROR, "Cannot allocate enough memory for ELF headers?!?\n");
        return ENOMEM;
    }

    memcpy(hdr, load, i);

    phdr = (Elf32_Phdr *) ((char *) hdr + hdr->e_phoff);
    /*shdr = (Elf32_Shdr*)(load + hdr->e_shoff); */
    *entrypoint = hdr->e_entry; /* MUST return this */

#if 0 // KYMAXXX allow kernel entrypoints
    /* Check if this is a kernel mode image */
    if (*entrypoint > MIPS_KSEG0_START) {
        printf("Cannot load image, entrypoint in kernel space: %#x\n",
               *entrypoint);
        return (EINVAL);
    }
#endif

    /* load all programs in this file */
    for (i = 0; i < hdr->e_phnum; i++, phdr++) {
        if (phdr->p_type != PT_LOAD)    /* only loadable types */
            continue;

#ifdef LOADER_DEBUG
        karma_log(DEBUG, "vaddr %#x, paddr %#x, load %#x, load+off %#x, msize %#x, fsize %#x\n",
               phdr->p_vaddr, MIPS_KSEG0_TO_PHYS(phdr->p_vaddr),
               load, load + phdr->p_offset, phdr->p_memsz, phdr->p_filesz);
#endif

        /* copy in the entire chunk of program */
        if (phdr->p_filesz > 0) {
#ifdef LOADER_DEBUG
            karma_log(DEBUG, "ELF_VADDR_TO_OFFSET(%p, 0x%08x, 0x%08lx): 0x%08lx\n", mem,
                   phdr->p_vaddr, vaddr_offset,
                   ELF_VADDR_TO_OFFSET(mem, phdr->p_vaddr, vaddr_offset));
#endif
            memmove((char *) ELF_VADDR_TO_OFFSET(mem, phdr->p_vaddr, vaddr_offset),
                    load + phdr->p_offset, phdr->p_filesz);
        }

        /* if in-memory size is larger, zero out the difference */
        if (phdr->p_memsz > phdr->p_filesz)
            memset((char *) ELF_VADDR_TO_OFFSET(mem, phdr->p_vaddr, vaddr_offset) +
                   phdr->p_filesz, 0, phdr->p_memsz - phdr->p_filesz);

        karma_log(INFO, " ... loaded vaddr 0x%08lx:%08lx @ addr 0x%08lx:%08lx\n",
            (l4_addr_t)phdr->p_vaddr, (l4_addr_t)(phdr->p_vaddr+phdr->p_memsz),
            (l4_addr_t)ELF_VADDR_TO_OFFSET(mem, phdr->p_vaddr, vaddr_offset),
            (l4_addr_t)ELF_VADDR_TO_OFFSET(mem, phdr->p_vaddr+phdr->p_memsz, vaddr_offset));
    }

    /*for (i = 0; i < hdr->e_shnum; i++, shdr++)
       load_section(shdr); */

    free(hdr);
    return 0;
}
