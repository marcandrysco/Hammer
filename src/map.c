#include "inc.h"

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
