#include "../inc.h"


/**
 * Create a reference.
 *   @spec: The special flag.
 *   @path: Consumed. The file path.
 *   &returns: The reference.
 */
struct target_t *rt_ref_new(bool spec, char *path)
{
	struct target_t *ref;

	ref = malloc(sizeof(struct target_t));
	*ref = (struct target_t){ path, spec ? FLAG_SPEC : 0, -1, NULL, NULL };

	return ref;
}

/**
 * Delete a reference.
 *   @ref: The reference.
 */
void rt_ref_delete(struct target_t *ref)
{
	struct edge_t *edge, *tmp;

	edge = ref->edge;
	while(edge != NULL) {
		edge = (tmp = edge)->next;
		free(tmp);
	}

	free(ref->path);
	free(ref);
}
