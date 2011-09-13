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
#include <arch/irq.h>
#include <arch/asm.h>
#include <arch/pic.h>
#include <arch/cpu.h>
#include <arch/page.h>
#include <kernel/kprintf.h>

/* TODO: Troche blokad? */

static struct irq_table _irq_table[IRQ_MAX];
static int _use_pic = 1;

void irq_use_pic(int use)
{
	_use_pic = use;
}

void irq_dispatcher(struct intr_stack * regs)
{
 int irq = regs->intr_no - IRQ_FIRST;
 if (irq > IRQ_MAX)
 {
  kprintf(KERN_WARN"IRQ: Oops, irq > IRQ_MAX!\n");
  return;
 }
 /* Aktualizacja statystyk */
 _irq_table[irq].count++;

 /* Wyślij EOI */
 if (_use_pic)
 {
  if (irq & 8)
  {
   outportb(PIC2, 0x60 + (irq & 7));
   outportb(PIC1, 0x62);
  }
  else
   outportb(PIC1, 0x60 + irq);
 }
 
 /* Wywołujemy handler przerwania */
 if (_irq_table[irq].handler)
    _irq_table[irq].handler(irq);
}

int irq_register(int irq, void * handler)
{
 if ((irq > IRQ_MAX) || (irq < 0))
    return -1;
 if (_irq_table[irq].handler)
    return -1;
 _irq_table[irq].handler = handler;

 if (_use_pic)
    pic_enable_irq(irq);
 return 0;
}

int irq_unregister(int irq)
{
 return -1;
}
