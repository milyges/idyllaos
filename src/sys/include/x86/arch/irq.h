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
#ifndef __ARCH_IRQ_H
#define __ARCH_IRQ_H

#define IRQ_FIRST   0x20
#define IRQ_MAX     24

#include <arch/intr.h>

struct irq_table
{
 void (*handler)(int irq);
 unsigned count;
};

void irq_use_pic(int use);
void irq_dispatcher(struct intr_stack * regs);
int irq_register(int irq, void * handler);
int irq_unregister(int irq);

#endif /* __ARCH_IRQ_H */
