#include "../inc.h"


/**
 * Create a binding assignment.
 *   @id: Consumed. The identifier.
 *   @tag: The tag.
 *   @data: Consumed. The data.
 *   @add: The append flag.
 *   &returns: The binding.
 */
struct ast_bind_t *ast_bind_new(struct raw_t *id, enum ast_bind_e tag, union ast_bind_u data, bool add)
{
	struct ast_bind_t *bind;

	bind = malloc(sizeof(struct ast_bind_t));
	bind->id = id;
	bind->tag = tag;
	bind->data = data;
	bind->add = add;

	return bind;
}

/**
 * Delete a binding assignment.
 *   @bind: The binding assignment.
 */
void ast_bind_delete(struct ast_bind_t *bind)
{
	switch(bind->tag) {
	case ast_val_v: imm_delete(bind->data.val); break;
	case ast_func_v: fatal("FIXME stub");
	case ast_block_v: ast_block_delete(bind->data.block); break;
	}

	raw_delete(bind->id);
	free(bind);
}


/**
 * Create a value binding assignemnt.
 *   @id: Consumed. The identifier.
 *   @val: Consumed. The value.
 *   @add: The append flag.
 *   &returns: The binding.
 */
struct ast_bind_t *ast_bind_val(struct raw_t *id, struct imm_t *val, bool add)
{
	return ast_bind_new(id, ast_val_v, (union ast_bind_u){ .val = val }, add);
}

/**
 * Create a block binding assignemnt.
 *   @id: Consumed. The identifier.
 *   @block: Consumed. The block.
 *   @add: The append flag.
 *   &returns: The binding.
 */
struct ast_bind_t *ast_bind_block(struct raw_t *id, struct ast_block_t *block, bool add)
{
	return ast_bind_new(id, ast_block_v, (union ast_bind_u){ .block = block }, add);
}
