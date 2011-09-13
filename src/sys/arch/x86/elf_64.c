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
#include <arch/types.h>
#include <kernel/kld.h>
#include <kernel/elf.h>
#include <kernel/kprintf.h>
#include <lib/errno.h>
#include <lib/string.h>
#include <mm/heap.h>

static inline char * elf64_section_name(void * image, int section)
{
	Elf64_Ehdr * header = image;
	Elf64_Shdr * sections = image + header->e_shoff;
	return image + sections[header->e_shstrndx].sh_offset + sections[section].sh_name;
}


static inline Elf64_Shdr * elf64_section_by_id(void * image, int section)
{
	Elf64_Ehdr * header = image;
	Elf64_Shdr * sections = image + header->e_shoff;
	if (section >= header->e_shnum)
		return NULL;

	return &sections[section];
}

static Elf64_Shdr * elf64_section_by_name(void * image, char * name)
{
	Elf64_Ehdr * header = image;
	int i;
	for (i=0;i<header->e_shnum;i++)
	{
		if (strcmp(elf64_section_name(image, i), name) == 0)
			return elf64_section_by_id(image, i);
	}
	return NULL;
}

static Elf64_Sym * elf64_symbol_by_name(void * image, char * name)
{
	Elf64_Shdr * sect = elf64_section_by_name(image, ".symtab");
	Elf64_Shdr * strtab = elf64_section_by_name(image, ".strtab");
	Elf64_Sym * symtab = image + sect->sh_offset;
	int i;
	int size = sect->sh_size / sizeof(Elf64_Sym);

	for (i=0;i<size;i++)
	{
		if (symtab[i].st_name == 0)
			continue;

		if (strcmp((char *)((addr_t)image + strtab->sh_offset + symtab[i].st_name), name) == 0)
			return &symtab[i];
	}
	return NULL;
}

int elf64_section(void * image, char * name, void ** ptr, size_t * size)
{
	Elf64_Shdr * sect = elf64_section_by_name(image, name);
	if (!sect)
		return -EFAULT;
	*ptr = (void *)(image + sect->sh_offset);
	*size = sect->sh_size;
	return 0;
}

static int elf64_lookup(struct kld_module * module, int index, uint64_t * val, uint32_t symtab_sect)
{
	Elf64_Shdr * sect;
	Elf64_Sym * sym;
	Elf64_Ehdr * elf = module->image;
	char * sym_name;

	if (symtab_sect >= elf->e_shnum)
		return -EFAULT;

	sect = elf64_section_by_id(module->image, symtab_sect);

	if (index >= sect->sh_size)
		return -EFAULT;

	sym = (Elf64_Sym *)(module->image + sect->sh_offset) + index;
	if (sym->st_shndx == 0) /* External symbol */
	{
		sect = elf64_section_by_id(module->image, sect->sh_link);
		sym_name = (char *)(module->image + sect->sh_offset + sym->st_name);
		*val = (addr_t)kld_import(sym_name, module);
		if (!*val)
		{
			kprintf("elf64: unknown symbol %s\n", sym_name);
			return -ENOTSUP;
		}
	}
	else
	{
		sect = elf64_section_by_id(module->image, sym->st_shndx);
		if (!sect)
			return -EFAULT;
		*val = (addr_t)(module->image + sym->st_value + sect->sh_offset);
	}
	return 0;
}

static int elf64_do_relocation(struct kld_module * module, Elf64_Rela * reloc, Elf64_Shdr * sect)
{
	Elf64_Shdr * target;
	uint64_t sym_val;
	uint64_t * where;
	int err;

	target = elf64_section_by_id(module->image, sect->sh_info);
	where = (uint64_t *)(module->image + target->sh_offset + reloc->r_offset);

	err = elf64_lookup(module, ELF64_R_SYM(reloc->r_info), &sym_val, sect->sh_link);
	if (err != 0)
		return err;

	if (sect->sh_type == SHT_RELA)
		sym_val += reloc->r_addend;

	switch (ELF64_R_TYPE(reloc->r_info))
	{
		/* R_X86_64 (S + A) */
		case 1: *where = sym_val + *where; break;
		/* R_X86_64_PC32 */
		//case 2: *where = sym_val + *where; break;
		default: return -ENOTSUP;
	}

	return 0;
}

static void * elf64_symbol(struct kld_module * module, char * name)
{
	Elf64_Sym * sym;
	Elf64_Shdr * sect;

	sym = elf64_symbol_by_name(module->image, name);
	if (!sym)
		return NULL;
	sect = elf64_section_by_id(module->image, sym->st_shndx);
	return (void *)(module->image + sym->st_value + sect->sh_offset);
}

static int elf64_relocate(struct kld_module * module)
{
	Elf64_Ehdr * elf = module->image;
	Elf64_Shdr * sect;
	Elf64_Rela * reloc;
	int i, j, reloc_size, err;

	/* Check for file format */
	if ((!IS_ELF(elf)) || (!ELF_CHECK(elf, ET_REL, EM_X86_64, ELFCLASS64, ELFDATA2LSB)))
		return -ENOEXEC;

	/* Search .bss secion and alloc memory */
	for (i=0;i<elf->e_shnum;i++)
	{
		sect = elf64_section_by_id(module->image, i);
		if(sect->sh_type != SHT_NOBITS) continue;
		module->bss = kalloc(sect->sh_size);
		memset(module->bss, 0x00, sect->sh_size);
		sect->sh_offset = (addr_t)module->bss - (addr_t)module->image;
	}

	/* For each section... */
	for (i=0;i<elf->e_shnum;i++)
	{
		sect = elf64_section_by_id(module->image, i);
		if (sect->sh_type == SHT_RELA)
			reloc_size = sizeof(Elf64_Rela);
		else if (sect->sh_type == SHT_REL)
			reloc_size = sizeof(Elf64_Rel);
		else
			continue;

		/* For each relocation... */
		for (j=0;j<sect->sh_size/reloc_size;j++)
		{
			reloc = (Elf64_Rela *)(module->image + sect->sh_offset + reloc_size * j);
			err = elf64_do_relocation(module, reloc, sect);
			if (err != 0)
				return err;
		}
	}

	return 0;
}


int elf_module(struct kld_module * module)
{
	int err;
	void * table;
	size_t table_len;
	char * tmp;

	err = elf64_relocate(module);
	if (err != 0)
		return err;

	module->info = elf64_symbol(module, "__module_info");
	if (!module->info)
		return -ENOEXEC;

	//module->info->module = module;

	err = elf64_section(module->image, "__export_table", &table, &table_len);
	if (!err)
	{
		tmp = table;
		while(tmp < (char *)(table + table_len))
		{
			if (!tmp)
			{
				tmp++;
				continue;
			}

			module->symtab_items++;
			module->symtab = krealloc(module->symtab, module->symtab_items * sizeof(struct kld_symbol));
			module->symtab[module->symtab_items - 1].name = tmp;
			module->symtab[module->symtab_items - 1].value = elf64_symbol(module, tmp);
			//kprintf(KERN_DEBUG"kld: exporting symbol %s -> 0x%p [module = %s]\n", tmp, module->symtab[module->symtab_items - 1].value, module->info->name);
			tmp += strlen(tmp) + 1;
		}
	}
	return 0;
}
