#include "inc.h"


/**
 * Create a new binding.
 *   @id: Consumed. The identifier.
 *   @obj: Consumed. The object.
 *   @loc: The location.
 *   &returns: The binding.
 */
struct bind_t *bind_new(char *id, struct rt_obj_t obj, struct loc_t loc)
{
	struct bind_t *bind;

	bind = malloc(sizeof(struct bind_t));
	*bind = (struct bind_t){ id, obj, loc, NULL };

	return bind;
}

/**
 * Delete a binding.
 *   @bind: The binding.
 */
void bind_delete(struct bind_t *bind)
{
	rt_obj_delete(bind->obj);
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
 * Reset the binding.
 *   @bind: The binding.
 *   @obj: Consumed. The object.
 *   @loc: The location.
 */
void bind_reset(struct bind_t *bind, struct rt_obj_t obj, struct loc_t loc)
{
	rt_obj_delete(bind->obj);
	bind->obj = obj;
}


/**
 * Create an object.
 *   @tag: The tag.
 *   @data: The data.
 *   &returns: The object.
 */
struct rt_obj_t rt_obj_new(enum rt_obj_e tag, union rt_obj_u data)
{
	return (struct rt_obj_t){ tag, data };
}

/**
 * Duplicate an object.
 *   @obj: The object.
 *   &returns: The duplicate.
 */
struct rt_obj_t rt_obj_dup(struct rt_obj_t obj)
{
	switch(obj.tag) {
	case rt_null_v: return rt_obj_null(); break;
	case rt_val_v: return rt_obj_val(val_dup(obj.data.val)); break;
	case rt_env_v: return rt_obj_env(rt_env_dup(obj.data.env)); break;
	case rt_func_v: return rt_obj_func(obj.data.func); break;
	}

	unreachable();
}

/**
 * Delete an object.
 *   @obj: The object.
 */
void rt_obj_delete(struct rt_obj_t obj)
{
	switch(obj.tag) {
	case rt_null_v: break;
	case rt_val_v: val_clear(obj.data.val); break;
	case rt_env_v: rt_env_clear(obj.data.env); break;
	case rt_func_v: break;
	}
}

/**
 * Set an object.
 *   @dst: The destination reference.
 *   @src: The source.
 */
void rt_obj_set(struct rt_obj_t *dst, struct rt_obj_t src)
{
	rt_obj_delete(*dst);
	*dst = src;
}


/**
 * Create a null object.
 *   &returns: The object.
 */
struct rt_obj_t rt_obj_null(void)
{
	return rt_obj_new(rt_null_v, (union rt_obj_u){ });
}

/**
 * Create a value object.
 *   @val: Consumed. The value.
 *   &returns: The object.
 */
struct rt_obj_t rt_obj_val(struct val_t *val)
{
	return rt_obj_new(rt_val_v, (union rt_obj_u){ .val = val });
}

/**
 * Create an environment object.
 *   @env: Consumed. The environment.
 *   &returns: The object.
 */
struct rt_obj_t rt_obj_env(struct env_t *env)
{
	return rt_obj_new(rt_env_v, (union rt_obj_u){ .env = env });
}

/**
 * Create a function object.
 *   @func: Consumed. The function.
 *   &returns: The object.
 */
struct rt_obj_t rt_obj_func(func_t *func)
{
	return rt_obj_new(rt_func_v, (union rt_obj_u){ .func = func });
}


/**
 * Add two objects.
 *   @dst: The destination object.
 *   @src: Consumed. The source object.
 */
void rt_obj_add(struct rt_obj_t dst, struct rt_obj_t src, struct loc_t loc)
{
	switch(dst.tag) {
	case rt_null_v:
		fatal("FIXME stub rt_obj_add null");
		break;

	case rt_val_v:
		if(src.tag != dst.tag)
			loc_err(loc, "Cannot add non-string value to a string value.");

		*val_tail(&dst.data.val) = src.data.val;
		break;

	case rt_env_v:
		if(src.tag != dst.tag)
			loc_err(loc, "Cannot add non-environment value to an environment value.");

		*rt_env_tail(&dst.data.env) = src.data.env;
		break;

	case rt_func_v:
		loc_err(loc, "Cannot add function to variable.");
		break;

	}
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
 * Retrieve the tail of a value.
 *   @val: The value reference.
 *   &return: The tail reference.
 */
struct val_t **val_tail(struct val_t **val)
{
	while(*val != NULL)
		val = &(*val)->next;

	return val;
}


/**
 * Create an environment.
 *   @up: The parent environment
 */
struct env_t *rt_env_new(struct env_t *up)
{
	struct env_t *env;

	env = malloc(sizeof(struct env_t));
	env->nrefs = 1;
	env->map = map0_new((cmp_f)strcmp, (del_f)bind_delete);
	env->next = up;

	return env;
}

/**
 * Duplicate an environment.
 *   @env: The environemnt.
 *   &returns: The duplicate.
 */
struct env_t *rt_env_dup(struct env_t *env)
{
	struct env_t *iter;

	for(iter = env; iter != NULL; iter = iter->next)
		iter->nrefs++;

	return env;
}

/**
 * Delete an environment.
 *   @env: The environment.
 */
void rt_env_delete(struct env_t *env)
{
	if(env->nrefs-- >= 2)
		return;

	map0_delete(env->map);
	free(env);
}

/**
 * Clear a list of environments.
 *   @env: The environments.
 */
void rt_env_clear(struct env_t *env)
{
	struct env_t *tmp;

	while(env != NULL) {
		env = (tmp = env)->next;
		rt_env_delete(tmp);
	}
}


/**
 * Lookup a binding from the current environment (not recursively).
 *   @env: The environment.
 *   @id: The identifier.
 *   &returns: The binding if found.
 */
struct bind_t *rt_env_lookup(struct env_t *env, const char *id)
{
	return map0_get(env->map, id);
}

/**
 * Get a binding from an environment.
 *   @env: The environment.
 *   @id: The identifier.
 *   &returns: The binding if found.
 */
struct bind_t *env_get(struct env_t *env, const char *id)
{
	struct bind_t *bind;

	while(env != NULL) {
		bind = map0_get(env->map, id);
		if(bind != NULL)
			return bind;

		env = env->next;
	}

	return NULL;
}

/**
 * Add a binding to an environment.
 *   @env: The environment.
 *   @bind: The binding.
 */
void env_put(struct env_t *env, struct bind_t *bind)
{
	struct bind_t *cur;

	cur = map_rem(env->map, bind->id);
	if(cur != NULL)
		bind_delete(cur);

	map0_add(env->map, bind->id, bind);
}

/**
 * Retrieve the tail of an environment.
 *   @env: The environment reference.
 *   &returns: The tail reference.
 */
struct env_t **rt_env_tail(struct env_t **env)
{
	while(*env != NULL)
		env = &(*env)->next;

	return env;
}
