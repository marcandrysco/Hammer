#include "inc.h"


/**
 * Create a new binding.
 *   @id: Consumed. The identifier.
 *   @tag: The tag.
 *   @data: Consumed. The data.
 *   &returns: The binding.
 */
struct bind_t *bind_new(char *id, enum bind_e tag, union bind_u data)
{
	struct bind_t *bind;

	bind = malloc(sizeof(struct bind_t));
	*bind = (struct bind_t){ id, tag, data, NULL };

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
	case rule_v: break;
	case ns_v: cli_err("FIXME stub");
	}

	if(bind->id != NULL)
		free(bind->id);

	free(bind);
}


/**
 * Create a value binding.
 *   @id: The identifier.
 *   @val: The value.
 *   &returns: The bidning.
 */
struct bind_t *bind_val(char *id, struct val_t *val)
{
	return bind_new(id, val_v, (union bind_u){ .val = val });
}

/**
 * Create a rule binding.
 *   @id: The identifier.
 *   @rule: The rule.
 *   &returns: The bidning.
 */
struct bind_t *bind_rule(char *id, struct rule_t *rule)
{
	return bind_new(id, rule_v, (union bind_u){ .rule = rule });
}


/**
 * Create a value.
 *   @str: Consumed. The string.
 *   &returns: The value.
 */
struct val_t *val_new(char *str)
{
	struct val_t *val;

	val = malloc(sizeof(struct val_t));
	val->str = str;
	val->next = NULL;

	return val;
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
