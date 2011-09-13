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
#ifndef __ARCH_CTX_H
#define __ARCH_CTX_H

#include <arch/spinlock.h>
#include <arch/intr.h>

/* Kontekst wątku */
struct ctx
{
	void * stackptr; /* Wskaźnik stosu kernela */
	void * stack; /* Adres stosu kernela */
	spinlock_t lock;
};

void ctx_switch(struct ctx * oldctx, struct ctx * newctx);
void ctx_init(struct ctx * ctx, void * entry);
void ctx_free(struct ctx * ctx);
void ctx_fork(struct ctx * newctx, struct intr_stack * stack);

#endif /* __ARCH_CTX_H */
