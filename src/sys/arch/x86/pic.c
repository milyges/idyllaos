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
#include <arch/asm.h>
#include <arch/pic.h>
#include <kernel/types.h>

static uint8_t _irq_mask[2] = { 0xFF, 0xFF };

void pic_enable_irq(int num)
{
 if (num < 8)
 {
  _irq_mask[0] &= ~(1 << num);
  outportb(PIC1 + 1, _irq_mask[0]);
 }
 else
 {
  _irq_mask[1] &= ~(1 << (num & 7));
  outportb(PIC2 + 1, _irq_mask[1]);
 }
}

void pic_disable_irq(int num)
{
 if (num < 8)
 {
  _irq_mask[0] |= 1 << num;
  outportb(PIC1 + 1, _irq_mask[0]);
 }
 else
 {
  _irq_mask[1] |= 1 << (num & 7);
  outportb(PIC2 + 1, _irq_mask[1]);
 }
}

void pic_init(void)
{
 /* Send ICW1 */
 outportb(PIC1, 0x11);
 outportb(PIC2, 0x11);

 /* Send ICW2 */
 outportb(PIC1 + 1, 0x20);
 outportb(PIC2 + 1, 0x28);

 /* Send ICW3 */
 outportb(PIC1 + 1, 4);
 outportb(PIC2 + 1, 2);

 /* Send ICW4 */
 outportb(PIC1 + 1, 0x01);
 outportb(PIC2 + 1, 0x01);

 /* Mask all IRQs */
 outportb(PIC1 + 1, _irq_mask[0]);
 outportb(PIC2 + 1, _irq_mask[1]);

 pic_enable_irq(2);
}

/* Mask all IRQs */
void pic_disable(void)
{
 _irq_mask[0] = 0xFF;
 _irq_mask[1] = 0xFF;
 outportb(PIC1 + 1, _irq_mask[0]);
 outportb(PIC2 + 1, _irq_mask[1]);
}
