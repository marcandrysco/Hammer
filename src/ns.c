#include "inc.h"


/**
 * Create a new namespace.
 *   @id: Optional. Consumed. The identifier.
 *   @up: Optional. The parent namespace.
 *   &returns: The namespace.
 */
struct ns_t *ns_new(char *id, struct ns_t *up)
{
	struct ns_t *ns;

	ns = malloc(sizeof(struct ns_t));
	ns->id = id;
	ns->up = up;
	ns->next = NULL;
	ns->bind = NULL;

	return ns;
}

/**
 * Delete a namespace.
 *   @ns: The namespace.
 */
void ns_delete(struct ns_t *ns)
{
	struct bind_t *bind;

	while(ns->bind != NULL) {
		ns->bind = (bind = ns->bind)->next;
		bind_delete(bind);
	}

	if(ns->id != NULL)
		free(ns->id);

	free(ns);
}


/**
 * Add a binding to the namespace.
 *   @ns: The namespace.
 *   @bind: The binding.
 */
void ns_add(struct ns_t *ns, struct bind_t *bind)
{
	if(bind->id == NULL) {
		bind->next = ns->bind;
		ns->bind = bind;
	}
	else {
		cli_err("FIXME STUB SLKFJSDLFJ");
	}
}


/**
 * Find a binding.
 *   @ns: The namespace.
 *   @id: The identifier.
 *   &returns: The binding if found, null otherwise.
 */
struct bind_t *ns_find(struct ns_t *ns, const char *id)
{
	return *ns_lookup(ns, id);
}

/**
 * Lookup a variable only on the current level.
 *   @ns: The namespace.
 *   @id: The identifier.
 *   &returns: The binding if found, point to end of list otherwise.
 */
struct bind_t **ns_lookup(struct ns_t *ns, const char *id)
{
	struct bind_t **bind;

	bind = &ns->bind;
	while(*bind != NULL) {
		if(strcmp((*bind)->id, id) == 0)
			break;

		bind = &(*bind)->next;
	}

	return bind;
}
