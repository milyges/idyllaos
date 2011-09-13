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
#include <arch/cpu.h>
#include <kernel/kld.h>
#include <kernel/elf.h>
#include <kernel/kprintf.h>
#include <kernel/proc.h>
#include <kernel/vfs.h>
#include <lib/errno.h>
#include <lib/string.h>
#include <mm/heap.h>

int elf_execve(int fd, struct vm_space * vmspace, void ** entry)
{
	Elf64_Ehdr hdr;

	/* Load elf header */
	sys_lseek(fd, 0, SEEK_SET, SCHED->current->proc);
	if (sys_read(fd, &hdr, sizeof(Elf64_Ehdr), SCHED->current->proc) != sizeof(Elf64_Ehdr))
		return -EIO;

	if (!IS_ELF(&hdr))
		return -ENOEXEC;
#if !defined(__X86_64__) || defined(__CONFIG_ENABLE_X86_SUPPORT)
	else if (ELF_CHECK(&hdr, ET_EXEC, EM_386, ELFCLASS32, ELFDATA2LSB))
		return elf32_execve(fd, vmspace, entry, (Elf32_Ehdr *)&hdr);
#endif /* !defined(__X86_64__) || defined(__CONFIG_ENABLE_X86_SUPPORT) */
#ifdef __X86_64__
	else if (ELF_CHECK(&hdr, ET_EXEC, EM_386, ELFCLASS64, ELFDATA2LSB))
	{
		kprintf("elf64 not suppported yet\n");
		//return elf64_execve(fd, vspace, entry, &hdr);
	}
#endif /* __X86_64__ */
	return -ENOEXEC;
}
