/*
 * Idylla Operating System
 * Copyright (C) 2009-2012 Idylla Operating System Team
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
#include <arch/spinlock.h>
#include <kernel/rwlock.h>
#include <lib/string.h>

void rwlock_init(struct rwlock * rw)
{
	memset(rw, 0x00, sizeof(struct rwlock));
	spinlock_init(&rw->lock);
	wait_init(&rw->wr_wait);
	wait_init(&rw->rd_wait);
}

void rwlock_lock_shared(struct rwlock * rw)
{
	spinlock_lock(&rw->lock);
	
	rw->rd_count++;
	if (rw->wr_count > 0)
		wait_sleep(&rw->rd_wait, &rw->lock);
	
	while(rw->active < 0)
		wait_sleep(&rw->rd_wait, &rw->lock);
	
	rw->active++;
	rw->rd_count--;
	
	spinlock_unlock(&rw->lock);
}

void rwlock_unlock_shared(struct rwlock * rw)
{
	spinlock_lock(&rw->lock);
	
	rw->active--;
	
	if (!rw->active)
		wait_wakeup(&rw->wr_wait);
	
	spinlock_unlock(&rw->lock);
}

void rwlock_lock_exclusive(struct rwlock * rw)
{
	spinlock_lock(&rw->lock);
	
	rw->wr_count++;
	while(rw->active)
		wait_sleep(&rw->wr_wait, &rw->lock);
	
	rw->wr_count--;
	rw->active = -1;
	
	spinlock_unlock(&rw->lock);
}

void rwlock_unlock_exclusive(struct rwlock * rw)
{
	spinlock_lock(&rw->lock);
	
	rw->active = 0;
	
	if (rw->rd_count > 0)
		wait_wakeup_all(&rw->rd_wait);
	else
		wait_wakeup(&rw->wr_wait);
	
	spinlock_unlock(&rw->lock);
}

void rwlock_upgrade(struct rwlock * rw)
{
	spinlock_lock(&rw->lock);
	
	if (rw->active == 1)
		rw->active = -1;
	else
	{
		rw->wr_count++;
		rw->active--;
		
		while(rw->active)
			wait_sleep(&rw->wr_wait, &rw->lock);
		
		rw->wr_count--;
		rw->active = -1;
	}
	
	spinlock_unlock(&rw->lock);
}

void rwlock_downgrade(struct rwlock * rw)
{
	spinlock_lock(&rw->lock);
	
	rw->active = 1;
	wait_wakeup_all(&rw->rd_wait);
	
	spinlock_unlock(&rw->lock);
}
