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
		cli_err("FIXE STUB SLKFJSDLFJ");
	}
}
