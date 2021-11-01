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


struct map1_t *map1_new(void (*cmp)(const void *, const void *), void (*del)(void *))
{
	return NULL;
}



#if 0
/**
 * Create a map.
 *   @init: The initial size.
 *   &returns: The map.
 */
struct map_t *map_new(uint32_t init)
{
	struct map_t *map;

	map = malloc(sizeof(struct map_t));
	map->cnt = 0;
	map->sz = init;
	map->arr = malloc(init * sizeof(void *));
	memset(map->arr, 0x00, init * sizeof(void *));

	map->alt = NULL;
	map->idx = 0;

	return map;
}

/**
 * Delete a map.
 *   @map: The map.
 */
void map_delete(struct map_t *map, size_t off, void(*del)(void *))
{
	uint32_t i;

	for(i = 0; i < map->sz; i++)
		ent_clear(map->arr[i], off, del);

	if(map->alt != NULL)
		map_delete(map->alt, off, del);

	free(map->arr);
	free(map);
}


/**
 * Retrieve an entity from the map.
 *   @map: The map.
 *   @hash: The hash.
 *   @ref: The reference.
 *   @off: The offset.
 *   @cmp: The comparison function.
 *   &returns: The entity if found, null if not found.
 */
struct ent_t *map_get(struct map_t *map, uint64_t hash, void *ref, ssize_t off, bool(*cmp)(const void *, const void *))
{
	struct ent_t *ent;

	for(ent = map->arr[hash % map->sz]; ent != NULL; ent = ent->next) {
		if((hash == ent->hash) && cmp(ref, (void *)ent - off))
			return ent;
	}

	return NULL;
}


/**
 * Add an entity to the map.
 *   @map: The map.
 *   @ent: The entity.
 *   @off: The offset.
 *   @cmp: The comparison function.
 *   &returns: True if added, false if already in map.
 */
bool map_add(struct map_t *map, struct ent_t *ent, ssize_t off, bool(*cmp)(const void *, const void *))
{
	struct ent_t *iter, **ref;

	ref = &map->arr[ent->hash % map->sz];
	for(iter = *ref; iter != NULL; iter = iter->next) {
		if((iter->hash == ent->hash) && cmp(ref, (void *)ent - off))
			return false;
	}

	ent->next = *ref;
	*ref = ent;

	return true;
}


/**
 * Determine if two entities are equal.
 *   @lhs: The left-hand side.
 *   @rhs: The right-hand side.
 *   @fn: The comparison function.
 *   &returns: True if equal, false otherwise.
 */
bool ent_equal(const struct ent_t *lhs, const struct ent_t *rhs, size_t off, bool(*fn)(void *,void *))
{
	return fn((void *)lhs - off, (void *)rhs - off);
}


/**
 * Create an entity.
 *   @hash: The hash.
 *   &returns: The entity.
 */
struct ent_t ent_new(uint64_t hash)
{
	return (struct ent_t){ hash, NULL };
}

/**
 * Delete an entity.
 *   @ent: The entity.
 *   @off: The offset to the parent.
 *   @del: The deletion callback.
 */
void ent_delete(struct ent_t *ent, size_t off, void(*del)(void *))
{
	del((void*)ent - off);
}

/**
 * Clear a list of entities.
 *   @ent: The entity list.
 *   @off: The offset to the parent.
 *   @del: The deletion callback.
 */
void ent_clear(struct ent_t *ent, size_t off, void(*del)(void *))
{
	struct ent_t *tmp;

	while(ent != NULL) {
		ent = (tmp = ent)->next;
		ent_delete(tmp, off, del);
	}
}
#endif
