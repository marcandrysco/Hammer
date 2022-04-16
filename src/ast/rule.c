#include "../inc.h"


/**
 * Create a syntatic rule.
 *   @gen: Consumed. The generator immediate.
 *   @dep: Consumed. The generator immediate.
 *   @loc: The location.
 *   &returns: The syntatic rule.
 */
struct ast_rule_t *ast_rule_new(struct imm_t *gen, struct imm_t *dep, struct loc_t loc)
{
	struct ast_rule_t *syn;

	syn = malloc(sizeof(struct ast_rule_t));
	syn->gen = gen;
	syn->dep = dep;
	syn->loc = loc;
	syn->cmd = NULL;

	return syn;
}

/**
 * Delete a syntatic rule.
 *   @syn: The syntatic rule.
 */
void ast_rule_delete(struct ast_rule_t *syn)
{
	if(syn->cmd != NULL)
		list_delete(syn->cmd);

	imm_delete(syn->gen);
	imm_delete(syn->dep);
	free(syn);
}


/**
 * Add a command to the syntatic rule.
 *   @syn: The syntax.
 *   @cmd: The command.
 */
void ast_rule_add(struct ast_rule_t *syn, struct imm_t *cmd)
{
	list_add(syn->cmd, cmd);
}
