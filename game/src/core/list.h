/*
 * list.h — Intrusive doubly-linked list library
 *
 * Type-safe circular doubly-linked lists with sentinel node.
 * Inspired by the Linux kernel's list.h, adapted for this codebase.
 *
 * Key properties:
 *   - Circular sentinel eliminates NULL checks and empty-list special cases
 *   - O(1) insert, remove, and empty check
 *   - Safe iteration allows removal of the current element during traversal
 *   - Count tracking maintained automatically
 *   - No allocation — list_node_t is embedded directly in your struct
 *
 * Usage:
 *   struct my_item {
 *       int value;
 *       list_node_t node;   // embed a list node
 *   };
 *
 *   list_head_t my_list;
 *   list_init(&my_list);
 *
 *   struct my_item item;
 *   list_push_back(&my_list, &item.node);
 *
 *   struct my_item *pos;
 *   LIST_FOR_EACH(pos, &my_list, struct my_item, node) {
 *       printf("%d\n", pos->value);
 *   }
 */

#ifndef LIST_H
#define LIST_H

#include <stdbool.h>
#include <stddef.h>

/*
 * List node — embed this in your struct.
 */
typedef struct list_node {
	struct list_node *next;
	struct list_node *prev;
} list_node_t;

/*
 * List head — contains a sentinel node and element count.
 * The sentinel's next points to the first element, prev to the last.
 * An empty list has both pointing back to the sentinel itself.
 */
typedef struct list_head {
	list_node_t sentinel;
	int count;
} list_head_t;

/*
 * Initialize a list head. Must be called before any other operations.
 */
static inline void list_init( list_head_t *head ) {
	head->sentinel.next = &head->sentinel;
	head->sentinel.prev = &head->sentinel;
	head->count = 0;
}

/*
 * Initialize a list node to a safe unlinked state.
 * A node initialized this way can be safely passed to list_node_is_linked().
 */
static inline void list_node_init( list_node_t *node ) {
	node->next = node;
	node->prev = node;
}

/*
 * Returns true if the node appears to be linked into a list.
 * A node is considered unlinked if both pointers point to itself
 * (as set by list_node_init or list_remove).
 */
static inline bool list_node_is_linked( const list_node_t *node ) {
	return node->next != node;
}

/*
 * Internal: splice a node between two adjacent nodes.
 */
static inline void list__insert_between( list_node_t *node,
										 list_node_t *prev_node,
										 list_node_t *next_node ) {
	next_node->prev = node;
	node->next = next_node;
	node->prev = prev_node;
	prev_node->next = node;
}

/*
 * Insert at the front of the list (after the sentinel).
 */
static inline void list_push_front( list_head_t *head, list_node_t *node ) {
	list__insert_between( node, &head->sentinel, head->sentinel.next );
	head->count++;
}

/*
 * Insert at the back of the list (before the sentinel).
 */
static inline void list_push_back( list_head_t *head, list_node_t *node ) {
	list__insert_between( node, head->sentinel.prev, &head->sentinel );
	head->count++;
}

/*
 * Insert node before a specific position.
 */
static inline void list_insert_before( list_head_t *head, list_node_t *node,
									   list_node_t *before ) {
	list__insert_between( node, before->prev, before );
	head->count++;
}

/*
 * Remove a node from its list. The node is reset to unlinked state.
 * The caller must pass the list head for count tracking.
 */
static inline void list_remove( list_head_t *head, list_node_t *node ) {
	node->prev->next = node->next;
	node->next->prev = node->prev;
	node->next = node;
	node->prev = node;
	head->count--;
}

/*
 * Returns true if the list contains no elements.
 */
static inline bool list_empty( const list_head_t *head ) {
	return head->sentinel.next == &head->sentinel;
}

/*
 * Returns the number of elements in the list.
 */
static inline int list_count( const list_head_t *head ) {
	return head->count;
}

/*
 * Get the first node, or NULL if the list is empty.
 */
static inline list_node_t *list_first( const list_head_t *head ) {
	return list_empty( head ) ? NULL : head->sentinel.next;
}

/*
 * Get the last node, or NULL if the list is empty.
 */
static inline list_node_t *list_last( const list_head_t *head ) {
	return list_empty( head ) ? NULL : head->sentinel.prev;
}

/*
 * Type-safe container_of — given a pointer to an embedded list_node_t,
 * recover a pointer to the containing struct.
 *
 * Example: LIST_ENTRY(node_ptr, struct my_item, node)
 */
#define LIST_ENTRY( ptr, type, member ) \
	( (type *) ( (char *) (ptr) - offsetof( type, member ) ) )

/*
 * Iterate over all elements in the list.
 * Do NOT remove elements during this iteration — use LIST_FOR_EACH_SAFE.
 *
 * After the loop, pos is set to NULL (matching old NULL-terminated behavior).
 * This prevents bugs where code checks `if (pos != NULL)` after the loop.
 *
 * @pos:    loop variable of type 'type *'
 * @head:   pointer to list_head_t
 * @type:   struct type containing the list_node_t
 * @member: name of the list_node_t field within the struct
 */
#define LIST_FOR_EACH( pos, head, type, member )                             \
	for ( (pos) = LIST_ENTRY( (head)->sentinel.next, type, member );         \
		  &(pos)->member != &(head)->sentinel                                \
			  || ( (pos) = NULL, 0 );                                        \
		  (pos) = LIST_ENTRY( (pos)->member.next, type, member ) )

/*
 * Iterate over all elements, safe for removal of the current element.
 *
 * After the loop, pos is set to NULL (matching old NULL-terminated behavior).
 *
 * @pos:    loop variable of type 'type *'
 * @tmp:    temporary variable of type 'type *' (used internally)
 * @head:   pointer to list_head_t
 * @type:   struct type containing the list_node_t
 * @member: name of the list_node_t field within the struct
 */
#define LIST_FOR_EACH_SAFE( pos, tmp, head, type, member )                   \
	for ( (pos) = LIST_ENTRY( (head)->sentinel.next, type, member ),         \
		  (tmp) = LIST_ENTRY( (pos)->member.next, type, member );            \
		  &(pos)->member != &(head)->sentinel                                \
			  || ( (pos) = NULL, 0 );                                        \
		  (pos) = (tmp),                                                     \
		  (tmp) = LIST_ENTRY( (tmp)->member.next, type, member ) )

/*
 * Iterate in reverse order (last to first).
 */
#define LIST_FOR_EACH_REVERSE( pos, head, type, member )                     \
	for ( (pos) = LIST_ENTRY( (head)->sentinel.prev, type, member );         \
		  &(pos)->member != &(head)->sentinel                                \
			  || ( (pos) = NULL, 0 );                                        \
		  (pos) = LIST_ENTRY( (pos)->member.prev, type, member ) )

/*
 * Iterate in reverse, safe for removal of the current element.
 */
#define LIST_FOR_EACH_REVERSE_SAFE( pos, tmp, head, type, member )           \
	for ( (pos) = LIST_ENTRY( (head)->sentinel.prev, type, member ),         \
		  (tmp) = LIST_ENTRY( (pos)->member.prev, type, member );            \
		  &(pos)->member != &(head)->sentinel                                \
			  || ( (pos) = NULL, 0 );                                        \
		  (pos) = (tmp),                                                     \
		  (tmp) = LIST_ENTRY( (tmp)->member.prev, type, member ) )

#endif /* LIST_H */
