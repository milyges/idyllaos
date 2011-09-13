/*
 * Idylla Operating System
 * Copyright (C) 2009-2010 Idylla Operating System Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
*/
#ifndef __KERNEL_ELF_H
#define __KERNEL_ELF_H

#include <kernel/types.h>
#include <kernel/kld.h>

/* Definicje niezależne od architektury  */

/* ELF Identification */
#define EI_MAG0        0 /* File identification */
#define EI_MAG1        1 /* File identification */
#define EI_MAG2        2 /* File identification */
#define EI_MAG3        3 /* File identification */
#define EI_CLASS       4 /* File class */
#define EI_DATA        5 /* Data encoding */
#define EI_VERSION     6 /* File version */
#define EI_PAD         7 /* Size of padding bytes */
#define EI_NIDENT      16 /* Size of e_ident[] */

/* ELF Magic number */
#define ELFMAG0        0x7F
#define ELFMAG1        'E'
#define ELFMAG2        'L'
#define ELFMAG3        'F'

/* e_ident[EI_CLASS] */
#define ELFCLASSNONE   0 /* Invalid class */
#define ELFCLASS32     1 /* 32-bit objects */
#define ELFCLASS64     2 /* 64-bit objects */

/* e_ident[EI_DATA] */
#define ELFDATANONE    0 /* Invalid data encoding */
#define ELFDATA2LSB    1
#define ELFDATA2MSB    2

/* ELF type */
#define ET_NONE        0 /* No file type */
#define ET_REL         1 /* Relocatable file */
#define ET_EXEC        2 /* Executable file */
#define ET_DYN         3 /* Shared object file */
#define ET_CORE        4 /* Core file */

/* ELF Machine */
#define EM_NONE		0	/* Unknown machine. */
#define EM_M32		1	/* AT&T WE32100. */
#define EM_SPARC	2	/* Sun SPARC. */
#define EM_386		3	/* Intel i386. */
#define EM_68K		4	/* Motorola 68000. */
#define EM_88K		5	/* Motorola 88000. */
#define EM_486		6	/* Intel i486. */
#define EM_860		7	/* Intel i860. */
#define EM_MIPS		8	/* MIPS R3000 Big-Endian only */
#define EM_S370		9	/* IBM System/370 */
#define EM_MIPS_RS4_BE	10	/* MIPS R4000 Big-Endian */ /* Depreciated */
#define EM_PARISC	15	/* HPPA */
#define EM_SPARC32PLUS	18	/* SPARC v8plus */
#define EM_PPC		20	/* PowerPC 32-bit */
#define EM_PPC64	21	/* PowerPC 64-bit */
#define EM_ARM		40	/* ARM */
#define EM_SPARCV9	43	/* SPARC v9 64-bit */
#define EM_IA_64	50	/* Intel IA-64 Processor */
#define EM_X86_64	62	/* Advanced Micro Devices x86-64 */
#define EM_ALPHA	0x9026	/* Alpha (written in the absence of an ABI */

/* ELF version */
#define EV_NONE        0 /* Invalid version */
#define EV_CURRENT     1 /* Current version */

/* Section types */
#define SHT_NULL       0
#define SHT_PROGBITS   1
#define SHT_SYMTAB     2
#define SHT_STRTAB     3
#define SHT_RELA       4
#define SHT_HASH       5
#define SHT_DYNAMIC    6
#define SHT_NOTE       7
#define SHT_NOBITS     8
#define SHT_REL        9
#define SHT_SHLIB     10
#define SHT_DYNSYM    11

/* Section Attribute Flags */
#define SHF_WRITE     0x001
#define SHF_ALLOC     0x002
#define SHF_EXECINSTR 0x004
#define SHF_MERGE     0x010
#define SHF_STRINGS   0x020

/* Symbol Binding */
#define STB_LOCAL     0
#define STB_GLOBAL    1
#define STB_WEAK      2
#define STB_LOOS      10

/* Symbol types */
#define STT_NOTYPE    0
#define STT_OBJECT    1
#define STT_FUNC      2
#define STT_SECTION   3
#define STT_FILE      4
#define STT_COMMON    5

/* Symbol Visibility */
#define STV_DEFAULT   0
#define STV_INTERNAL  1
#define STV_HIDDEN    2
#define STV_PROTECTED 3

/* Values for p_type. */
#define PT_NULL       0 /* Unused entry. */
#define PT_LOAD       1 /* Loadable segment. */
#define PT_DYNAMIC    2 /* Dynamic linking information segment. */
#define PT_INTERP     3 /* Pathname of interpreter. */
#define PT_NOTE       4 /* Auxiliary information. */
#define PT_SHLIB      5 /* Reserved (not used). */
#define PT_PHDR       6 /* Location of program header itself. */
#define PT_TLS        7 /* Thread local storage segment */

#define PT_COUNT      8 /* Number of defined p_type values. */

#define PT_LOOS       0x60000000 /* OS-specific */
#define PT_HIOS       0x6fffffff /* OS-specific */
#define PT_LOPROC     0x70000000 /* First processor-specific type. */
#define PT_HIPROC     0x7fffffff /* Last processor-specific type. */

/* Values for p_flags. */
#define PF_X          0x1 /* Executable. */
#define PF_W          0x2 /* Writable. */
#define PF_R          0x4 /* Readable. */

#define IS_ELF(ehdr)  ((ehdr)->e_ident[EI_MAG0] == ELFMAG0 && \
                       (ehdr)->e_ident[EI_MAG1] == ELFMAG1 && \
                       (ehdr)->e_ident[EI_MAG2] == ELFMAG2 && \
                       (ehdr)->e_ident[EI_MAG3] == ELFMAG3)

#define ELF_CHECK(elf,type,machine,class,endian) \
                      (((elf)->e_type == type) && ((elf)->e_machine == machine) && \
                      ((elf)->e_ident[EI_CLASS] == class) && ((elf)->e_ident[EI_DATA] == endian))


#include <mm/vmm.h>
#include <arch/elf.h>

int elf_module(struct kld_module * module);
int elf_execve(int fd, struct vm_space * vmspace, void ** entry);

#endif /* __KERNEL_ELF_H */
