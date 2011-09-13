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
#ifndef __LIB_LIST_H
#define __LIB_LIST_H

typedef struct list
{
	struct list * next;
	struct list * prev;
} list_t;

#define LIST_INIT(name)         { &(name), &(name) }
#define LIST_NEW(name)          list_t name = LIST_INIT(name)
#define LIST_IS_EMPTY(list)     (((list)->prev == (list)) && ((list)->next == (list)))

#define LIST_FOREACH(list,item) \
        for(item = (typeof(item))(list)->next; item != (typeof(item))(list);item = (typeof(item))(((list_t *)(item))->next))

#define LIST_FOREACH_SAFE(list,item) \
	list_t * __iter_ ## item; \
	for(__iter_ ## item = (list)->next, item = (typeof(item))__iter_ ## item; \
	    __iter_ ## item != (list); __iter_ ## item = (__iter_ ## item)->next, item = (typeof(block))__iter_ ## item)

static inline void list_init(list_t * l)
{
	l->next = l;
	l->prev = l;
}

static inline void list_add(list_t * l, list_t * i)
{
	/* Usuń ze starej listy */
	i->prev->next = i->next;
	i->next->prev = i->prev;

	/* Dodaj do nowej listy */
	i->prev = l;
	i->next = l->next;
	l->next->prev = i;
	l->next = i;
}

static inline void list_remove(list_t * i)
{
	/* Usuń ze starej listy */
	i->prev->next = i->next;
	i->next->prev = i->prev;
	/* Stwórz "nową" liste */
	i->next = i->prev = i;
}

#endif /* __LIB_LIST_H */
