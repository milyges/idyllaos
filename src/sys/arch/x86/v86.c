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
#include <arch/atomic.h>
#include <arch/intr.h>
#include <arch/cpu.h>
#include <arch/page.h>
#include <kernel/types.h>
#include <kernel/kprintf.h>
#include <kernel/debug.h>
#include <kernel/proc.h>
#include <lib/string.h>
#include <mm/heap.h>
#include <mm/kmem.h>

#ifdef __CONFIG_ENABLE_V86
/*
static inline addr_t to_linear(unsigned seg, unsigned off)
{
	return (seg & 0xFFFF) * 16L + off;
}

static inline unsigned peekb(unsigned seg, unsigned off)
{
	return *(uint8_t *)(to_linear(seg, off) + KERNEL_BASE_ADDR);
}

static inline unsigned peekw(unsigned seg, unsigned off)
{
	return *(uint16_t *)(to_linear(seg, off) + KERNEL_BASE_ADDR);
}

static inline static uint32_t peekl(unsigned seg, unsigned off)
{
	return *(uint32_t *)(to_linear(seg, off) + KERNEL_BASE_ADDR);
}

static inline void pokeb(unsigned seg, unsigned off, unsigned val)
{
	*(uint8_t *)(to_linear(seg, off) + KERNEL_BASE_ADDR) = val;
}

static inline static void pokew(unsigned seg, unsigned off, unsigned val)
{
	*(uint16_t *)(to_linear(seg, off) + KERNEL_BASE_ADDR) = val;
}

static inline static void pokel(unsigned seg, unsigned off, uint32_t val)
{
	*(uint32_t *)(to_linear(seg, off) + KERNEL_BASE_ADDR) = val;
}


static inline void v86_push16(struct cpu_context * ctx, unsigned value)
{
 ctx->esp = (ctx->esp - 2) & 0xFFFF;
 pokew(ctx->ss, ctx->esp, value);
}

static unsigned v86_pop16(struct cpu_context * ctx)
{
 unsigned rv;

 rv = peekw(ctx->ss, ctx->esp);
 ctx->esp = (ctx->esp + 2) & 0xFFFF;
 return rv;
}

static void v86_push32(struct cpu_context * ctx, uint32_t value)
{
 ctx->esp = (ctx->esp - 4) & 0xFFFF;
 pokel(ctx->ss, ctx->esp, value);
}

static uint32_t v86_pop32(struct cpu_context * ctx)
{
 uint32_t rv;

 rv = peekl(ctx->ss, ctx->esp);
 ctx->esp = (ctx->esp + 4) & 0xFFFF;
 return rv;
}

static unsigned v86_fetch8(struct cpu_context * ctx)
{
 unsigned byte;

 byte = peekb(ctx->cs, ctx->eip);
 ctx->eip = (ctx->eip + 1) & 0xFFFF;
 return byte;
}*/

void v86_init(void)
{
	kprintf("v86: starting virtual 8086 mode monitor\n");

}

#endif /* __CONFIG_ENABLE_V86 */
