#include "inc.h"


/**
 * Set structure.
 *   @elem: The element.
 */
struct set_t {
	struct elem_t *elem;
};

/**
 * Element structure.
 *   @ref: The reference.
 *   @next: The next element.
 */
struct elem_t {
	void *ref;
	struct elem_t *next;
};


/**
 * Create a set.
 *   &returns: The set.
 */
struct set_t *set_new(void)
{
	struct set_t *set;

	set = malloc(sizeof(struct set_t));
	set->elem = NULL;

	return set;
}

/**
 * Delete a set.
 *   @set; The set.
 *   @del: The element deletion function.
 */
void set_delete(struct set_t *set, void(*del)(void *))
{
	struct elem_t *elem, *tmp;

	elem = set->elem;
	while(elem != NULL) {
		elem = (tmp = elem)->next;
		
		if(del != NULL)
			del(tmp->ref);

		free(tmp);
	}

	free(set);
}


/**
 * Add an element to a map.
 *   @map: The map.
 *   @ref: The reference.
 */
void set_add(struct set_t *set, void *ref)
{
	struct elem_t *elem;

	elem = malloc(sizeof(struct elem_t));
	elem->ref = ref;

	elem->next = set->elem;
	set->elem = elem;
}

/**
 * Create a list.
 *   @del: The value deletion function.
 *   &returns: The list.
 */
struct list_t *list_new(void(*del)(void *))
{
	struct list_t *list;

	list = malloc(sizeof(struct list_t));
	list->head = NULL;
	list->tail = &list->head;
	list->del = del;

	return list;
}

/**
 * Delete a list.
 *   @list: The list.
 */
void list_delete(struct list_t *list)
{
	struct link_t *link, *tmp;

	link = list->head;
	while(link != NULL) {
		link = (tmp = link)->next;
		
		if(list->del != NULL)
			list->del(tmp->val);

		free(tmp);
	}

	free(list);
}


/**
 * Add a value to a list.
 *   @list: The list.
 *   @val: The value.
 */
void list_add(struct list_t *list, void *val)
{
	struct link_t *link;

	link = malloc(sizeof(struct link_t));
	link->val = val;
	link->next = NULL;

	*list->tail = link;
	list->tail = &link->next;
}
