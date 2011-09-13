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
#include <arch/ctx.h>
#include <arch/cpu.h>
#include <kernel/types.h>
#include <kernel/proc.h>
#include <mm/heap.h>

extern void __ctx_switch(struct ctx * oldctx, struct ctx * newctx);

void ctx_switch(struct ctx * oldctx, struct ctx * newctx)
{
	/* Blokujemy struktury */
	spinlock_lock(&oldctx->lock);
	spinlock_lock(&newctx->lock);

	/* Aktualizacja TSS */
	CPU->tss.esp0 = (addr_t)(SCHED->current->ctx.stack + __CONFIG_KSTACK_SIZE);

	/* TODO: Przełączenie kontekstu fpu */

	/* Wywołujemy niskopoziomową funkcję która zmieni stosy i odblokuje struktury */
	__ctx_switch(oldctx, newctx);
}

void ctx_fork(struct ctx * newctx, struct intr_stack * stack)
{
	reg_t * ptr;

	newctx->stack = kalloc(__CONFIG_KSTACK_SIZE);
	ptr = (reg_t *)(newctx->stack + __CONFIG_KSTACK_SIZE);
	//kprintf("newctx->stack = %x\n", newctx->stack);
	//kprintf("ptr=%x\n", ptr);

	*--ptr = stack->sp;
	*--ptr = stack->ip;
	*--ptr = 0;
	*--ptr = (reg_t)&execve_user; /* Punkt początkowy */
	*--ptr = stack->bp; /* Zapisane ebp */
	*--ptr = CPU_IF | 0x02; /* Flagi procesora */
	*--ptr = stack->ax; /* (e|r)ax */
	*--ptr = stack->cx; /* (e|r)cx */
	*--ptr = stack->dx; /* (e|r)dx */
	*--ptr = stack->bx; /* (e|r)bx */
	*--ptr = stack->si; /* (e|r)si */
	*--ptr = stack->di; /* (e|r)di */

#ifdef __X86_64__
	*--ptr = stack->r8; /* r8 */
	*--ptr = stack->r9; /* r9 */
	*--ptr = stack->r10; /* r10 */
	*--ptr = stack->r11; /* r11 */
	*--ptr = stack->r12; /* r12 */
	*--ptr = stack->r13; /* r13 */
	*--ptr = stack->r14; /* r14 */
	*--ptr = stack->r15; /* r15 */
#endif /* __X86_64__ */

	newctx->stackptr = ptr;

}

void ctx_init(struct ctx * ctx, void * entry)
{
	reg_t * ptr;

	spinlock_init(&ctx->lock);

	ctx->stack = kalloc(__CONFIG_KSTACK_SIZE);

	ptr = (reg_t *)(ctx->stack + __CONFIG_KSTACK_SIZE);
	*--ptr = 0;
	*--ptr = (reg_t)entry; /* Punkt początkowy */
	*--ptr = 0; /* Zapisane ebp */
	*--ptr = CPU_IF | 0x02; /* Flagi procesora */
	*--ptr = 0; /* (e|r)ax */
	*--ptr = 0; /* (e|r)cx */
	*--ptr = 0; /* (e|r)dx */
	*--ptr = 0; /* (e|r)bx */
	*--ptr = 0; /* (e|r)si */
	*--ptr = 0; /* (e|r)di */

#ifdef __X86_64__
	*--ptr = 0; /* r8 */
	*--ptr = 0; /* r9 */
	*--ptr = 0; /* r10 */
	*--ptr = 0; /* r11 */
	*--ptr = 0; /* r12 */
	*--ptr = 0; /* r13 */
	*--ptr = 0; /* r14 */
	*--ptr = 0; /* r15 */
#endif /* __X86_64__ */

	ctx->stackptr = ptr;
}


void ctx_free(struct ctx * ctx)
{
	kfree(ctx->stack);
	ctx->stack = NULL;
	ctx->stackptr = NULL;
}
