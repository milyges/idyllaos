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
#ifndef __KERNEL_MODULE_H
#define __KERNEL_MODULE_H

struct __module_info
{
	char * name;
	int (*init)(int argc, char * argv[]);
	int (*clean)(void);
};

#ifdef __MODULE__

#define MODULE_INFO(n,i,c) struct __module_info __module_info = { n, i, c }
#define MODULE_EXPORT(s)   const char __export_##s[] __attribute__((section("__export_table"), aligned(1))) = #s

#endif /* __MODULE__ */

#endif /* __KERNEL_MODULE_H */