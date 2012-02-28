/*
 * Copyright © 2010-2012 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#ifndef LIST_H
#define LIST_H

#include <stdbool.h>

struct list {
    struct list *next, *prev;
};

static void
list_init(struct list *list)
{
    list->next = list->prev = list;
}

static inline void
__list_add(struct list *entry,
	    struct list *prev,
	    struct list *next)
{
    next->prev = entry;
    entry->next = next;
    entry->prev = prev;
    prev->next = entry;
}

static inline void
list_add(struct list *entry, struct list *head)
{
    __list_add(entry, head, head->next);
}

static inline void
list_add_tail(struct list *entry, struct list *head)
{
    __list_add(entry, head->prev, head);
}

static inline void list_replace(struct list *old,
				struct list *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}

static inline void
list_append(struct list *entry, struct list *head)
{
    __list_add(entry, head->prev, head);
}


static inline void
__list_del(struct list *prev, struct list *next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void
_list_del(struct list *entry)
{
    __list_del(entry->prev, entry->next);
}

static inline void
list_del(struct list *entry)
{
    _list_del(entry);
    list_init(entry);
}

static inline void list_move(struct list *list, struct list *head)
{
	if (list->prev != head) {
		_list_del(list);
		list_add(list, head);
	}
}

static inline void list_move_tail(struct list *list, struct list *head)
{
	_list_del(list);
	list_add_tail(list, head);
}

static inline bool
list_is_empty(struct list *head)
{
    return head->next == head;
}

#undef container_of
#define container_of(ptr, type, member) \
	((type *)((char *)(ptr) - (char *) &((type *)0)->member))

#define __container_of(ptr, sample, member)				\
    (void *)((char *)(ptr)						\
	     - ((char *)&(sample)->member - (char *)(sample)))

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)

#define list_last_entry(ptr, type, member) \
    list_entry((ptr)->prev, type, member)

#define list_for_each(pos, head)				\
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_entry(pos, head, member)				\
    for (pos = __container_of((head)->next, pos, member);		\
	 &pos->member != (head);					\
	 pos = __container_of(pos->member.next, pos, member))

#define list_for_each_entry_safe(pos, tmp, head, member)		\
    for (pos = __container_of((head)->next, pos, member),		\
	 tmp = __container_of(pos->member.next, pos, member);		\
	 &pos->member != (head);					\
	 pos = tmp, tmp = __container_of(pos->member.next, tmp, member))

#endif /* LIST_H */

