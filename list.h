#ifndef _AH_LIST_H
#define _AH_LIST_H

#include <stddef.h>

typedef struct list_head {
	struct list_head *next, *prev;
} list_head;

#define container_of(ptr, type, member) ({ \
        const typeof( ((type *)0)->member ) *__mptr = (ptr); \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define list_entry(ptr, type, member) \
        container_of(ptr, type, member)

#define list_next_entry(pos, member) \
        list_entry((pos)->member.next, typeof(*(pos)), member)

#define list_for_each(pos, head) \
        for (pos = (head); pos; pos = pos->next != (head) ? pos->next : NULL)

#define list_for_each_entry(pos, head, member) \
        for (pos = list_entry(head, typeof(*pos), member); \
             pos; \
             pos = pos->member.next != (head) ? list_next_entry(pos, member) : NULL)

static inline void INIT_LIST_HEAD(struct list_head *list) {
	list->next = list;
	list->prev = list;
}

static inline void __list_add(list_head *new, list_head *prev, list_head *next) {
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void list_add(list_head *new, list_head *head) {
	__list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}

#endif
