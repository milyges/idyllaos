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
#include <kernel/types.h>
#include <kernel/kld.h>
#include <kernel/elf.h>
#include <kernel/kprintf.h>
#include <kernel/vfs.h>
#include <mm/heap.h>
#include <mm/vmm.h>
#include <lib/string.h>
#include <lib/errno.h>
#include <lib/math.h>

#ifndef __X86_64__

static inline char * elf32_section_name(void * image, int section)
{
	Elf32_Ehdr * header = image;
	Elf32_Shdr * sections = image + header->e_shoff;
	return image + sections[header->e_shstrndx].sh_offset + sections[section].sh_name;
}

static inline Elf32_Shdr * elf32_section_by_id(void * image, int section)
{
	Elf32_Ehdr * header = image;
	Elf32_Shdr * sections = image + header->e_shoff;
	if (section >= header->e_shnum)
		return NULL;

	return &sections[section];
}

static Elf32_Shdr * elf32_section_by_name(void * image, char * name)
{
	Elf32_Ehdr * header = image;
	int i;
	for (i=0;i<header->e_shnum;i++)
	{
		if (strcmp(elf32_section_name(image, i), name) == 0)
			return elf32_section_by_id(image, i);
	}
	return NULL;
}

static Elf32_Sym * elf32_symbol_by_name(void * image, char * name)
{
	Elf32_Shdr * sect = elf32_section_by_name(image, ".symtab");
	Elf32_Shdr * strtab = elf32_section_by_name(image, ".strtab");
	Elf32_Sym * symtab = image + sect->sh_offset;
	int i;
	int size = sect->sh_size / sizeof(Elf32_Sym);

	for (i=0;i<size;i++)
	{
		if (symtab[i].st_name == 0)
			continue;

		if (strcmp((char *)((addr_t)image + strtab->sh_offset + symtab[i].st_name), name) == 0)
			return &symtab[i];
	}
	return NULL;
}

int elf32_section(void * image, char * name, void ** ptr, size_t * size)
{
	Elf32_Shdr * sect = elf32_section_by_name(image, name);
	if (!sect)
		return -EFAULT;
	*ptr = (void *)(image + sect->sh_offset);
	*size = sect->sh_size;
	return 0;
}

static int elf32_lookup(struct kld_module * module, int index, uint32_t * val, uint32_t symtab_sect)
{
	Elf32_Shdr * sect;
	Elf32_Sym * sym;
	Elf32_Ehdr * elf = module->image;
	char * sym_name;

	if (symtab_sect >= elf->e_shnum)
		return -EFAULT;

	sect = elf32_section_by_id(module->image, symtab_sect);

	if (index >= sect->sh_size)
		return -EFAULT;

	sym = (Elf32_Sym *)(module->image + sect->sh_offset) + index;
	if (sym->st_shndx == 0) /* External symbol */
	{
		sect = elf32_section_by_id(module->image, sect->sh_link);
		sym_name = (char *)(module->image + sect->sh_offset + sym->st_name);
		*val = (addr_t)kld_import(sym_name, module);
		if (!*val)
		{
			kprintf("elf32: unknown symbol %s\n", sym_name);
			return -ENOTSUP;
		}
	}
	else
	{
		sect = elf32_section_by_id(module->image, sym->st_shndx);
		if (!sect)
			return -EFAULT;
		*val = (addr_t)(module->image + sym->st_value + sect->sh_offset);
	}
	return 0;
}

static int elf32_do_relocation(struct kld_module * module, Elf32_Rela * reloc, Elf32_Shdr * sect)
{
	Elf32_Shdr * target;
	uint32_t sym_val;
	uint32_t * where;
	int err;

	target = elf32_section_by_id(module->image, sect->sh_info);
	where = (uint32_t *)(module->image + target->sh_offset + reloc->r_offset);

	err = elf32_lookup(module, ELF32_R_SYM(reloc->r_info), &sym_val, sect->sh_link);
	if (err != 0)
		return err;

	if (sect->sh_type == SHT_RELA)
		sym_val += reloc->r_addend;

	switch (ELF32_R_TYPE(reloc->r_info))
	{
		/* R_386_32 (S + A) */
		case 1: *where = sym_val + *where; break;
		/* R_386_PC32 (S + A - P) */
		case 2: *where = sym_val + *where - (addr_t)where; break;
		default: return -ENOTSUP;
	}

	return 0;
}

static void * elf32_symbol(struct kld_module * module, char * name)
{
	Elf32_Sym * sym;
	Elf32_Shdr * sect;

	sym = elf32_symbol_by_name(module->image, name);
	if (!sym)
		return NULL;
	sect = elf32_section_by_id(module->image, sym->st_shndx);
	return (void *)(module->image + sym->st_value + sect->sh_offset);
}

int elf32_relocate(struct kld_module * module)
{
	Elf32_Ehdr * elf = module->image;
	Elf32_Shdr * sect;
	Elf32_Rela * reloc;
	int i, j, reloc_size, err;

	/* Sprawdzamy format pliku */
	if ((!IS_ELF(elf)) || (!ELF_CHECK(elf, ET_REL, EM_386, ELFCLASS32, ELFDATA2LSB)))
		return -ENOEXEC;

	/* Szukamy sekcji .bss i alokujemy pamięć dla niej */
	for (i=0;i<elf->e_shnum;i++)
	{
		sect = elf32_section_by_id(module->image, i);
		if(sect->sh_type != SHT_NOBITS)
			continue;
		module->bss = kalloc(sect->sh_size);
		memset(module->bss, 0, sect->sh_size);
		sect->sh_offset = (addr_t)module->bss - (addr_t)module->image;
	}

	/* Dla każdej sekcji... */
	for (i=0;i<elf->e_shnum;i++)
	{
		sect = elf32_section_by_id(module->image, i);
		if (sect->sh_type == SHT_RELA)
			reloc_size = sizeof(Elf32_Rela);
		else if (sect->sh_type == SHT_REL)
			reloc_size = sizeof(Elf32_Rel);
		else
			continue;

		/* Dla każdej relokacji */
		for (j=0;j<sect->sh_size/reloc_size;j++)
		{
			reloc = (Elf32_Rela *)(module->image + sect->sh_offset + reloc_size * j);
			err = elf32_do_relocation(module, reloc, sect);
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

	err = elf32_relocate(module);
	if (err != 0)
		return err;

	module->info = elf32_symbol(module, "__module_info");
	if (!module->info)
		return -ENOEXEC;

	//module->info->module = module;

	err = elf32_section(module->image, "__export_table", &table, &table_len);
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
			module->symtab[module->symtab_items - 1].value = elf32_symbol(module, tmp);
			//kprintf(KERN_DEBUG"kld: exporting symbol %s -> 0x%p [module = %s]\n", tmp, module->symtab[module->symtab_items - 1].value, module->info->name);
			tmp += strlen(tmp) + 1;
		}
	}
	return 0;
}

#endif /* __X86_64__ */

#if !defined(__X86_64__) || defined(__CONFIG_ENABLE_X86_SUPPORT)

/* Loadable segment support */
static int elf32_ph_load(int fd, Elf32_Phdr * seg, struct vm_space * vmspace)
{
	addr_t addr, off;
	size_t lenfile, lenmem;
	struct vm_space * tmp;
	unsigned prot = PROT_WRITE;

	if (!seg->p_memsz) /* Nie ma nic do roboty */
		return 0;

	addr = ROUND_DOWN(seg->p_vaddr, PAGE_SIZE);
	if (seg->p_offset < seg->p_vaddr - addr)
		return -EFAULT;

	if (seg->p_flags & PF_X)
		prot |= PROT_EXEC;

	if (seg->p_flags & PF_W)
		prot |= PROT_WRITE;

	if (seg->p_flags & PF_R)
		prot |= PROT_READ;

	off = seg->p_offset - (seg->p_vaddr - addr);
	lenfile = ROUND_UP((seg->p_vaddr - addr) + seg->p_filesz, PAGE_SIZE);
	lenmem = ROUND_UP(seg->p_memsz, PAGE_SIZE) - lenfile;
	
	//kprintf("elf32_execve: vaddr=%x, offset=%x, filesz=%x, memsz=%x\n", seg->p_vaddr, seg->p_offset, seg->p_filesz, seg->p_memsz);
	if (lenfile > 0)
	{
		//kprintf(" -> mmap file: addr=%x, off=%x, len=%x\n", addr, off, lenfile);
		vm_mmap((void *)addr, lenfile, prot | MAP_FIXED | MAP_PRIVATE, fd, off, vmspace, NULL);
	}

	if (lenfile > seg->p_filesz)
	{
		//kprintf(" -> zero: addr=%x, len=%x\n", seg->p_vaddr + seg->p_filesz, lenfile - seg->p_filesz);
		sched_preempt_disable();
		tmp = CPU->vmspace;
		vm_space_switch(vmspace);
		memset((void *)(seg->p_vaddr + seg->p_filesz), 0, (lenfile - seg->p_filesz - (seg->p_vaddr - addr)));
		vm_space_switch(tmp);
		sched_preempt_enable();
	}

	
	if (lenmem > 0)
	{
	//	kprintf(" -> mmap anon: addr=%x, len=%x\n", addr + lenfile, lenmem);
		vm_mmap((void *)addr + lenfile, lenmem, prot | MAP_FIXED | MAP_ANONYMOUS, -1, 0, vmspace, NULL);
	}

	return 0;
}

/* interp segment support */
static int elf32_ph_interp(int fd, Elf32_Phdr * seg, struct vm_space * vmspace, void ** entry)
{
	return -ENOSYS;
}


int elf32_execve(int fd, struct vm_space * vmspace, void ** entry, Elf32_Ehdr * hdr)
{
	void * buf;
	ssize_t err = 0;
	Elf32_Phdr * seg;
	int i;

	/* Ladujemy naglowki programowe */
	buf = kalloc(hdr->e_phentsize * hdr->e_phnum);
	sys_lseek(fd, hdr->e_phoff, SEEK_SET, SCHED->current->proc);
	err = sys_read(fd, buf, hdr->e_phentsize * hdr->e_phnum, SCHED->current->proc);
	if (err < 0)
		goto end;
	else if (err != hdr->e_phentsize * hdr->e_phnum)
	{
		err = -ENOEXEC;
		goto end;
	}

	/* Zapisujemy entry point */
	*entry = (void *)hdr->e_entry;

	for(i=0;i<hdr->e_phnum;i++)
	{
		seg = (Elf32_Phdr *)(buf + i * hdr->e_phentsize);
		switch(seg->p_type)
		{
			case PT_LOAD: /* Segment ladowalny */
			{
				err = elf32_ph_load(fd, seg, vmspace);
				if (err != 0)
					goto end;
				break;
			}
			case PT_INTERP: /* Dynamic linker path */
			{
				err = elf32_ph_interp(fd, seg, vmspace, entry);
				if (err != 0)
					goto end;
				break;
			}
			case PT_DYNAMIC:
			case PT_PHDR: break;
			default:
			{
				kprintf(KERN_DEBUG"elf32: unexpected segment (type=%u)\n", seg->p_type);
				//err = -ENOEXEC;
				//goto end;
			}
		}
	}

end:
	kfree(buf);
	return err;
}


#endif /* !defined(__X86_64__) || defined(__CONFIG_ENABLE_X86_SUPPORT) */
