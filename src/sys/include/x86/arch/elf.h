/*
 * Idylla Operating System
 * Copyright (C) 2009  Idylla Operating System Team
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
#ifndef __ARCH_ELF_H
#define __ARCH_ELF_H

#ifndef __KERNEL_ELF_H
#error "Include kernel/elf.h instead of arch/elf.h"
#endif /* __KERNEL_ELF_H */

/* ELF Header */
typedef struct
{
	uint8_t e_ident[EI_NIDENT];  /* File identification. */
	uint16_t e_type;             /* File type. */
	uint16_t e_machine;          /* Machine architecture. */
	uint32_t e_version;          /* ELF format version. */
	uint32_t e_entry;            /* Entry point. */
	uint32_t e_phoff;            /* Program header file offset. */
	uint32_t e_shoff;            /* Section header file offset. */
	uint32_t e_flags;            /* Architecture-specific flags. */
	uint16_t e_ehsize;           /* Size of ELF header in bytes. */
	uint16_t e_phentsize;        /* Size of program header entry. */
	uint16_t e_phnum;            /* Number of program header entries. */
	uint16_t e_shentsize;        /* Size of section header entry. */
	uint16_t e_shnum;            /* Number of section header entries. */
	uint16_t e_shstrndx;         /* Section name strings section. */
} Elf32_Ehdr;

typedef struct
{
	uint8_t e_ident[EI_NIDENT];
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint64_t e_entry;
	uint64_t e_phoff;
	uint64_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
} Elf64_Ehdr;

/* Section Header */
typedef struct
{
	uint32_t sh_name;
	uint32_t sh_type;
	uint32_t sh_flags;
	uint32_t sh_addr;
	uint32_t sh_offset;
	uint32_t sh_size;
	uint32_t sh_link;
	uint32_t sh_info;
	uint32_t sh_addralign;
	uint32_t sh_entsize;
} Elf32_Shdr;

typedef struct
{
	uint32_t sh_name;
	uint32_t sh_type;
	uint64_t sh_flags;
	uint64_t sh_addr;
	uint64_t sh_offset;
	uint64_t sh_size;
	uint32_t sh_link;
	uint32_t sh_info;
	uint64_t sh_addralign;
	uint64_t sh_entsize;
} Elf64_Shdr;

/* Elf symbol */
typedef struct
{
	uint32_t st_name;
	uint32_t st_value;
	uint32_t st_size;
	uint8_t st_info;
	uint8_t st_other;
	uint16_t st_shndx;
} Elf32_Sym;

typedef struct
{
	uint32_t st_name;
	uint8_t st_info;
	uint8_t st_other;
	uint16_t st_shndx;
	uint64_t st_value;
	uint64_t st_size;
} Elf64_Sym;

/* Relocation Entries */
typedef struct
{
	uint32_t r_offset;
	uint32_t r_info;
} Elf32_Rel;

typedef struct
{
	uint32_t r_offset;
	uint32_t r_info;
	int32_t r_addend;
} Elf32_Rela;

typedef struct
{
	uint64_t r_offset;
	uint64_t r_info;
} Elf64_Rel;

typedef struct
{
	uint64_t r_offset;
	uint64_t r_info;
	int64_t r_addend;
} Elf64_Rela;

/* Program header */
typedef struct
{
	uint32_t p_type;
	uint32_t p_offset;
	uint32_t p_vaddr;
	uint32_t p_paddr;
	uint32_t p_filesz;
	uint32_t p_memsz;
	uint32_t p_flags;
	uint32_t p_align;
} Elf32_Phdr;

#define ELF32_ST_BIND(i)       ((i) >> 4)
#define ELF32_ST_TYPE(i)       ((i) & 0xF)
#define ELF32_ST_INFO(b,t)     (((b) << 4) + ((t) & 0xF))
#define ELF32_ST_VISIBILITY(o) ((o) & 0x3)

#define ELF64_ST_BIND(i)       ((i) >> 4)
#define ELF64_ST_TYPE(i)       ((i) & 0xF)
#define ELF64_ST_INFO(b,t)     (((b) << 4) + ((t) & 0xF))
#define ELF64_ST_VISIBILITY(o) ((o) & 0x3)

#define ELF32_R_SYM(i)         ((i) >> 8)
#define ELF32_R_TYPE(i)        ((unsigned char)(i))
#define ELF32_R_INFO(s,t)      (((s) << 8) + (unsigned char)(t))

#define ELF64_R_SYM(i)    ((i) >> 32)
#define ELF64_R_TYPE(i)   ((i) & 0xFFFFFFFFL)
#define ELF64_R_INFO(s,t) (((s) << 32) + ((t) & 0xFFFFFFFFL))

int elf32_execve(int fd, struct vm_space * vmspace, void ** entry, Elf32_Ehdr * hdr);

#endif /* __ARCH_ELF_H */
