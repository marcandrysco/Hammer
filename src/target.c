#include "inc.h"

/**
 * Retrieve the target modification time, caching it as needed.
 *   @target: The target.
 *   &returns: The modification time.
 */
int64_t target_mtime(struct target_t *target)
{
	if(target->mtime < 0)
		target->mtime = os_mtime(target->path);

	return target->mtime;
}


/**
 * Connect a target to a rule.
 *   @target: The target.
 *   @rule: The rule.
 */
void target_conn(struct target_t *target, struct rule_t *rule)
{
	struct edge_t *edge;

	edge = malloc(sizeof(struct edge_t));
	edge->rule = rule;
	edge->next = target->edge;
	target->edge = edge;
}


/**
 * Retrieve an iterator to the target list.
 *   @list: The list.
 *   &returns: The iterator.
 */
struct target_iter_t target_iter(struct target_list_t *list)
{
	return (struct target_iter_t){ list->inst };
}

/**
 * Retrieve the next target.
 *   @iter: The iterater.
 *   &returns: The target or null.
 */
struct target_t *target_next(struct target_iter_t *iter)
{
	struct target_t *target;

	if(iter->inst == NULL)
		return NULL;

	target = iter->inst->target;
	iter->inst = iter->inst->next;
	return target;
}


/**
 * Create a target list.
 *   &returns: The list.
 */
struct target_list_t *target_list_new(void)
{
	struct target_list_t *list;

	list = malloc(sizeof(struct target_list_t));
	*list = (struct target_list_t){ NULL };

	return list;
}

/**
 * Destroy a target list.
 *   @list: The list.
 */
void target_list_delete(struct target_list_t *list)
{
	struct target_inst_t *inst, *tmp;

	inst = list->inst;
	while(inst != NULL) {
		inst = (tmp = inst)->next;
		free(tmp);
	}

	free(list);
}


/**
 * Retrieve the list length.
 *   @list: The list.
 *   &returns: The length.
 */
uint32_t target_list_len(struct target_list_t *list)
{
	uint32_t n = 0;
	struct target_inst_t *inst;

	for(inst = list->inst; inst != NULL; inst = inst->next)
		n++;

	return n;
}

/**
 * Check if the list contains a target.
 *   @list: The list.
 *   @target: The target.
 *   &returns: True if target exists within the list.
 */
bool target_list_contains(struct target_list_t *list, struct target_t *target)
{
	struct target_inst_t *inst;

	for(inst = list->inst; inst != NULL; inst = inst->next) {
		if(inst->target == target)
			return true;
	}

	return false;
}

/**
 * Add a target to the list.
 *   @list: The list.
 *   @target: The target.
 */
void target_list_add(struct target_list_t *list, struct target_t *target)
{
	struct target_inst_t **inst;

	inst = &list->inst;
	while(*inst != NULL)
		inst = &(*inst)->next;

	*inst = malloc(sizeof(struct target_inst_t));
	(*inst)->target = target;
	(*inst)->next = NULL;
}

struct target_t *target_list_find(struct target_list_t *list, bool spec, const char *path)
{
	struct target_inst_t *inst;

	for(inst = list->inst; inst != NULL; inst = inst->next) {
		if((strcmp(inst->target->path, path) == 0))
			return inst->target;
	}

	return NULL;
}


/**
 * Create a target map.
 *   &returns: The map.
 */
struct map_t *map_new(void)
{
	struct map_t *map;

	map = malloc(sizeof(struct map_t));
	map->ent = NULL;

	return map;
}

/**
 * Delete a target map.
 *   @map: The map.
 */
void map_delete(struct map_t *map)
{
	struct ent_t *ent, *tmp;

	ent = map->ent;
	while(ent != NULL) {
		ent = (tmp = ent)->next;
		rt_ref_delete(tmp->target);
		free(tmp);
	}

	free(map);
}


/**
 * Retrieve a target from a map.
 *   @map: The map.
 *   @spec: The special flag.
 *   @path: The path.
 *   &returns: The target or null if not found.
 */
struct target_t *map_get(struct map_t *map, bool spec, const char *path)
{
	struct ent_t *ent;

	for(ent = map->ent; ent != NULL; ent = ent->next) {
		if(strcmp(ent->target->path, path) == 0)
			return ent->target;
	}

	return NULL;
}

/**
 * Add a target to map.
 *   @map: The map.
 *   @target: Consumed. The target.
 */
void map_add(struct map_t *map, struct target_t *target)
{
	struct ent_t *ent;

	ent = malloc(sizeof(struct ent_t));
	ent->target = target;

	ent->next = map->ent;
	map->ent = ent;
}
