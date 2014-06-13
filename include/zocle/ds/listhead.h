#ifndef ZOCLE_DS_LISTHEAD_H
#define ZOCLE_DS_LISTHEAD_H

#include <stdio.h>
#include <stdint.h>

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef container_of
#define container_of(ptr, type, member) ({                      \
            const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
            (type *)( (char *)__mptr - offsetof(type,member) );})
#endif

#define   LIST_POISON1  ((void *) 0x00100100)
#define   LIST_POISON2  ((void *) 0x00200200)

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */


struct zc_listhead_t {
    struct zc_listhead_t *next, *prev;
};

typedef struct zc_listhead_t zcListHead;

#define ZC_LISTHEAD_INIT(name) { &(name), &(name) }

#define ZC_LISTHEAD(name) \
	struct zc_listhead_t name = ZC_LISTHEAD_INIT(name)

static inline void zc_listhead_init(struct zc_listhead_t *list)
{
	list->next = list;
	list->prev = list;
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void zc_listhead_add_bt(struct zc_listhead_t *new,
			      struct zc_listhead_t *prev,
			      struct zc_listhead_t *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void zc_listhead_add(struct zc_listhead_t *new, struct zc_listhead_t *head)
{
	zc_listhead_add_bt(new, head, head->next);
}


/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void zc_listhead_add_tail(struct zc_listhead_t *new, struct zc_listhead_t *head)
{
	zc_listhead_add_bt(new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void zc_listhead_del_bt(struct zc_listhead_t * prev, struct zc_listhead_t * next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void zc_listhead_del_entry(struct zc_listhead_t *entry)
{
	zc_listhead_del_bt(entry->prev, entry->next);
}

static inline void zc_listhead_del_set(struct zc_listhead_t *entry)
{
	zc_listhead_del_bt(entry->prev, entry->next);
	entry->next = LIST_POISON1;
	entry->prev = LIST_POISON2;
}

/**
 * list_replace - replace old entry by new one
 * @old : the element to be replaced
 * @new : the new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static inline void zc_listhead_replace(struct zc_listhead_t *old,
				struct zc_listhead_t *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}

static inline void zc_listhead_replace_init(struct zc_listhead_t *old,
					struct zc_listhead_t *new)
{
	zc_listhead_replace(old, new);
	zc_listhead_init(old);
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static inline void zc_listhead_del_init(struct zc_listhead_t *entry)
{
	zc_listhead_del_entry(entry);
	zc_listhead_init(entry);
}

/**
 * list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static inline void zc_listhead_move(struct zc_listhead_t *list, struct zc_listhead_t *head)
{
	zc_listhead_del_entry(list);
	zc_listhead_add(list, head);
}

/**
 * list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static inline void zc_listhead_move_tail(struct zc_listhead_t *list,
				  struct zc_listhead_t *head)
{
	zc_listhead_del_entry(list);
	zc_listhead_add_tail(list, head);
}

/**
 * list_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static inline int zc_listhead_is_last(const struct zc_listhead_t *list,
				const struct zc_listhead_t *head)
{
	return list->next == head;
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int zc_listhead_empty(const struct zc_listhead_t *head)
{
	return head->next == head;
}

/**
 * list_empty_careful - tests whether a list is empty and not being modified
 * @head: the list to test
 *
 * Description:
 * tests whether a list is empty _and_ checks that no other CPU might be
 * in the process of modifying either member (next or prev)
 *
 * NOTE: using list_empty_careful() without synchronization
 * can only be safe if the only activity that can happen
 * to the list entry is list_del_init(). Eg. it cannot be used
 * if another CPU could re-list_add() it.
 */
static inline int zc_listhead_empty_careful(const struct zc_listhead_t *head)
{
	struct zc_listhead_t *next = head->next;
	return (next == head) && (next == head->prev);
}

/**
 * list_rotate_left - rotate the list to the left
 * @head: the head of the list
 */
static inline void zc_listhead_rotate_left(struct zc_listhead_t *head)
{
	struct zc_listhead_t *first;

	if (!zc_listhead_empty(head)) {
		first = head->next;
		zc_listhead_move_tail(first, head);
	}
}

/**
 * list_is_singular - tests whether a list has just one entry.
 * @head: the list to test.
 */
static inline int zc_listhead_is_singular(const struct zc_listhead_t *head)
{
	return !zc_listhead_empty(head) && (head->next == head->prev);
}

static inline void zc_listhead_cut_position_internal(struct zc_listhead_t *list,
		struct zc_listhead_t *head, struct zc_listhead_t *entry)
{
	struct zc_listhead_t *new_first = entry->next;
	list->next = head->next;
	list->next->prev = list;
	list->prev = entry;
	entry->next = list;
	head->next = new_first;
	new_first->prev = head;
}

/**
 * list_cut_position - cut a list into two
 * @list: a new list to add all removed entries
 * @head: a list with entries
 * @entry: an entry within head, could be the head itself
 *	and if so we won't cut the list
 *
 * This helper moves the initial part of @head, up to and
 * including @entry, from @head to @list. You should
 * pass on @entry an element you know is on @head. @list
 * should be an empty list or a list you do not care about
 * losing its data.
 *
 */
static inline void list_cut_position(struct zc_listhead_t *list,
		struct zc_listhead_t *head, struct zc_listhead_t *entry)
{
	if (zc_listhead_empty(head))
		return;
	if (zc_listhead_is_singular(head) &&
		(head->next != entry && head != entry))
		return;
	if (entry == head)
	zc_listhead_init(list);
	else
		zc_listhead_cut_position_internal(list, head, entry);
}

static inline void zc_listhead_splice_internal(const struct zc_listhead_t *list,
				 struct zc_listhead_t *prev,
				 struct zc_listhead_t *next)
{
	struct zc_listhead_t *first = list->next;
	struct zc_listhead_t *last = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

/**
 * list_splice - join two lists, this is designed for stacks
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void zc_listhead_splice(const struct zc_listhead_t *list,
				struct zc_listhead_t *head)
{
	if (!zc_listhead_empty(list))
		zc_listhead_splice_internal(list, head, head->next);
}

/**
 * list_splice_tail - join two lists, each list being a queue
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void zc_listhead_splice_tail(struct zc_listhead_t *list,
				struct zc_listhead_t *head)
{
	if (!zc_listhead_empty(list))
		zc_listhead_splice_internal(list, head->prev, head);
}

/**
 * list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
static inline void zc_listhead_splice_init(struct zc_listhead_t *list,
				    struct zc_listhead_t *head)
{
	if (!zc_listhead_empty(list)) {
		zc_listhead_splice_internal(list, head, head->next);
		zc_listhead_init(list);
	}
}

/**
 * list_splice_tail_init - join two lists and reinitialise the emptied list
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * Each of the lists is a queue.
 * The list at @list is reinitialised
 */
static inline void zc_listhead_splice_tail_init(struct zc_listhead_t *list,
					 struct zc_listhead_t *head)
{
	if (!zc_listhead_empty(list)) {
		zc_listhead_splice_internal(list, head->prev, head);
		zc_listhead_init(list);
	}
}

/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct zc_listhead_t pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define zc_listhead_entry(ptr, type, member) \
	container_of(ptr, type, member)

/**
 * list_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define zc_listhead_first_entry(ptr, type, member) \
	zc_listhead_entry((ptr)->next, type, member)

/**
 * list_last_entry - get the last element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define zc_listhead_last_entry(ptr, type, member) \
	zc_listhead_entry((ptr)->prev, type, member)

/**
 * list_first_entry_or_null - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 *
 * Note that if the list is empty, it returns NULL.
 */
#define zc_listhead_first_entry_or_null(ptr, type, member) \
	(!zc_listhead_empty(ptr) ? zc_listhead_first_entry(ptr, type, member) : NULL)

/**
 * list_next_entry - get the next element in list
 * @pos:	the type * to cursor
 * @member:	the name of the list_struct within the struct.
 */
#define zc_listhead_next_entry(pos, member) \
	zc_listhead_entry((pos)->member.next, typeof(*(pos)), member)

/**
 * list_prev_entry - get the prev element in list
 * @pos:	the type * to cursor
 * @member:	the name of the list_struct within the struct.
 */
#define zc_listhead_prev_entry(pos, member) \
	zc_listhead_entry((pos)->member.prev, typeof(*(pos)), member)

/**
 * list_for_each	-	iterate over a list
 * @pos:	the &struct zc_listhead_t to use as a loop cursor.
 * @head:	the head for your list.
 */
#define zc_listhead_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_prev	-	iterate over a list backwards
 * @pos:	the &struct zc_listhead_t to use as a loop cursor.
 * @head:	the head for your list.
 */
#define zc_listhead_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

/**
 * list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:	the &struct zc_listhead_t to use as a loop cursor.
 * @n:		another &struct zc_listhead_t to use as temporary storage
 * @head:	the head for your list.
 */
#define zc_listhead_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

/**
 * list_for_each_prev_safe - iterate over a list backwards safe against removal of list entry
 * @pos:	the &struct zc_listhead_t to use as a loop cursor.
 * @n:		another &struct zc_listhead_t to use as temporary storage
 * @head:	the head for your list.
 */
#define zc_listhead_for_each_prev_safe(pos, n, head) \
	for (pos = (head)->prev, n = pos->prev; \
	     pos != (head); \
	     pos = n, n = pos->prev)

/**
 * list_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define zc_listhead_for_each_entry(pos, head, member)				\
	for (pos = zc_listhead_first_entry(head, typeof(*pos), member);	\
	     &pos->member != (head);					\
	     pos = zc_listhead_next_entry(pos, member))

/**
 * list_for_each_entry_reverse - iterate backwards over list of given type.
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define zc_listhead_for_each_entry_reverse(pos, head, member)			\
	for (pos = list_last_entry(head, typeof(*pos), member);		\
	     &pos->member != (head); 					\
	     pos = zc_listhead_prev_entry(pos, member))

/**
 * list_prepare_entry - prepare a pos entry for use in list_for_each_entry_continue()
 * @pos:	the type * to use as a start point
 * @head:	the head of the list
 * @member:	the name of the list_struct within the struct.
 *
 * Prepares a pos entry for use as a start point in list_for_each_entry_continue().
 */
#define zc_listhead_prepare_entry(pos, head, member) \
	((pos) ? : zc_listhead_entry(head, typeof(*pos), member))

/**
 * list_for_each_entry_continue - continue iteration over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Continue to iterate over list of given type, continuing after
 * the current position.
 */
#define zc_listhead_for_each_entry_continue(pos, head, member) 		\
	for (pos = zc_listhead_next_entry(pos, member);			\
	     &pos->member != (head);					\
	     pos = zc_listhead_next_entry(pos, member))

/**
 * list_for_each_entry_continue_reverse - iterate backwards from the given point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Start to iterate over list of given type backwards, continuing after
 * the current position.
 */
#define zc_listhead_for_each_entry_continue_reverse(pos, head, member)		\
	for (pos = zc_listhead_prev_entry(pos, member);			\
	     &pos->member != (head);					\
	     pos = zc_listhead_prev_entry(pos, member))

/**
 * list_for_each_entry_from - iterate over list of given type from the current point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate over list of given type, continuing from current position.
 */
#define zc_listhead_for_each_entry_from(pos, head, member) 			\
	for (; &pos->member != (head);					\
	     pos = zc_listhead_next_entry(pos, member))

/**
 * list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define zc_listhead_for_each_entry_safe(pos, n, head, member)			\
	for (pos = zc_listhead_first_entry(head, typeof(*pos), member),	\
		n = zc_listhead_next_entry(pos, member);			\
	     &pos->member != (head); 					\
	     pos = n, n = zc_listhead_next_entry(n, member))

/**
 * zc_listhead_for_each_entry_safe_continue - continue list iteration safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the zc_listhead_struct within the struct.
 *
 * Iterate over list of given type, continuing after current point,
 * safe against removal of list entry.
 */
#define zc_listhead_for_each_entry_safe_continue(pos, n, head, member) 		\
	for (pos = zc_listhead_next_entry(pos, member), 				\
		n = zc_listhead_next_entry(pos, member);				\
	     &pos->member != (head);						\
	     pos = n, n = zc_listhead_next_entry(n, member))

/**
 * zc_listhead_for_each_entry_safe_from - iterate over list from current point safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the zc_listhead_struct within the struct.
 *
 * Iterate over list of given type from current point, safe against
 * removal of list entry.
 */
#define zc_listhead_for_each_entry_safe_from(pos, n, head, member) 			\
	for (n = zc_listhead_next_entry(pos, member);					\
	     &pos->member != (head);						\
	     pos = n, n = zc_listhead_next_entry(n, member))

/**
 * zc_listhead_for_each_entry_safe_reverse - iterate backwards over list safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the zc_listhead_struct within the struct.
 *
 * Iterate backwards over list of given type, safe against removal
 * of list entry.
 */
#define zc_listhead_for_each_entry_safe_reverse(pos, n, head, member)		\
	for (pos = zc_listhead_last_entry(head, typeof(*pos), member),		\
		n = zc_listhead_prev_entry(pos, member);			\
	     &pos->member != (head); 					\
	     pos = n, n = zc_listhead_prev_entry(n, member))

/**
 * zc_listhead_safe_reset_next - reset a stale zc_listhead_for_each_entry_safe loop
 * @pos:	the loop cursor used in the zc_listhead_for_each_entry_safe loop
 * @n:		temporary storage used in zc_listhead_for_each_entry_safe
 * @member:	the name of the zc_listhead_struct within the struct.
 *
 * zc_listhead_safe_reset_next is not safe to use in general if the list may be
 * modified concurrently (eg. the lock is dropped in the loop body). An
 * exception to this is if the cursor element (pos) is pinned in the list,
 * and zc_listhead_safe_reset_next is called after re-taking the lock and before
 * completing the current iteration of the loop body.
 */
#define zc_listhead_safe_reset_next(pos, n, member)				\
	n = zc_listhead_next_entry(pos, member)


#endif
