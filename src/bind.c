#include "inc.h"


/**
 * Create a new binding.
 *   @id: Consumed. The identifier.
 *   @tag: The tag.
 *   @data: Consumed. The data.
 *   @loc: The location.
 *   &returns: The binding.
 */
struct bind_t *bind_new(char *id, enum bind_e tag, union bind_u data, struct loc_t loc)
{
	struct bind_t *bind;

	bind = malloc(sizeof(struct bind_t));
	*bind = (struct bind_t){ id, tag, data, loc, NULL };

	return bind;
}

/**
 * Delete a binding.
 *   @bind: The binding.
 */
void bind_delete(struct bind_t *bind)
{
	switch(bind->tag) {
	case val_v: val_clear(bind->data.val); break;
	case func_v: break;
	case ns_v: cli_err("FIXME stub");
	}

	free(bind->id);
	free(bind);
}

/**
 * Delete a binding if non-null.
 *   @bind: The bind.
 */
void bind_erase(struct bind_t *bind)
{
	if(bind != NULL)
		bind_delete(bind);
}


/**
 * Set a new binding, replacing the old value.
 *   @dst: The destination.
 *   @src: The source.
 */
void bind_set(struct bind_t **dst, struct bind_t *src)
{
	if(*dst != NULL)
		bind_delete(*dst);

	*dst = src;
}


/**
 * Create a value binding.
 *   @id: The identifier.
 *   @val: The value.
 *   @loc: The location.
 *   &returns: The binding.
 */
struct bind_t *bind_val(char *id, struct val_t *val)
{
	return bind_new(id, val_v, (union bind_u){ .val = val }, (struct loc_t){ });
}

/**
 * Create a function binding.
 *   @id: The identifier.
 *   @rule: The rule.
 *   &returns: The binding.
 */
struct bind_t *bind_func(char *id, func_t *func)
{
	return bind_new(id, func_v, (union bind_u){ .func = func }, (struct loc_t){ });
}


/**
 * Create a value.
 *   @spec: Special value flag.
 *   @str: Consumed. The string.
 *   &returns: The value.
 */
struct val_t *val_new(bool spec, char *str)
{
	struct val_t *val;

	val = malloc(sizeof(struct val_t));
	val->spec = spec;
	val->str = str;
	val->next = NULL;

	return val;
}

/**
 * Duplicate a value.
 *   @val: The value.
 *   &returns: The duplicated value.
 */
struct val_t *val_dup(const struct val_t *val)
{
	struct val_t *ret, **iter;

	iter = &ret;
	while(val != NULL) {
		*iter = malloc(sizeof(struct val_t));
		(*iter)->spec = val->spec;
		(*iter)->str = strdup(val->str);
		iter = &(*iter)->next;
		val = val->next;
	}

	*iter = NULL;
	return ret;
}

/**
 * Delete a value.
 *   @val: The value.
 */
void val_delete(struct val_t *val)
{
	free(val->str);
	free(val);
}

/**
 * Clear a list of values.
 *   @val: The value list.
 */
void val_clear(struct val_t *val)
{
	struct val_t *tmp;

	while(val != NULL) {
		val = (tmp = val)->next;
		val_delete(tmp);
	}
}


/**
 * Unwrap an identifier from a value.
 *   @val: Consumed. The value.
 *   @loc: The location.
 *   &returns: The allocated identifier.
 */
char *val_id(struct val_t *val, struct loc_t loc)
{
	char *id;

	if((val == NULL) || (val_len(val) >= 2))
		loc_err(loc, "Invalid variable name.");

	id = val->str;
	free(val);

	return id;
}

/**
 * Unwrap an single string from a value.
 *   @val: Consumed. The value.
 *   @loc: The location.
 *   &returns: The allocated identifier.
 */
char *val_str(struct val_t *val, struct loc_t loc)
{
	char *id;

	if((val == NULL) || (val_len(val) >= 2))
		loc_err(loc, "Must be a single string.");

	id = val->str;
	free(val);

	return id;
}

/**
 * Retrieve the value length.
 *   @val: The value.
 *   &returns: The length.
 */
uint32_t val_len(struct val_t *val)
{
	uint32_t n = 0;

	while(val != NULL) {
		n++;
		val = val->next;
	}

	return n;
}


/**
 * Finalize a value by setting the directory.
 *   @val: The value.
 *   @dir: The directory.
 */
void val_final(struct val_t *val, const char *dir)
{
	while(val != NULL) {
		str_final(&val->str, dir);
		val = val->next;
	}
}
