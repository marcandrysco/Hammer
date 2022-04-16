#include "../inc.h"


/**
 * Create a block.
 *   &returns: The block.
 */
struct ast_block_t *ast_block_new(void)
{
	struct ast_block_t *block;

	block = malloc(sizeof(struct ast_block_t));
	block->stmt = NULL;

	return block;
}

/**
 * Delete a block.
 *   @block: The block.
 */
void ast_block_delete(struct ast_block_t *block)
{
	stmt_clear(block->stmt);
	free(block);
}
