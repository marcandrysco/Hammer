#include "../inc.h"


struct ast_env_t {
	struct block_t *block;
};


/**
 * Create a new processing statement.
 *   @pipe: Consumed. The pipe list.
 *   &returns: The processing statement.
 */
struct ast_cmd_t *ast_cmd_new(struct ast_pipe_t *pipe)
{
	struct ast_cmd_t *proc;

	proc = malloc(sizeof(struct ast_cmd_t));
	proc->pipe = pipe;
	proc->in = proc->out = NULL;
	proc->append = false;

	return proc;
}

/**
 * Delete a processing statement.
 *   @proc: The processing statement.
 */
void ast_cmd_delete(struct ast_cmd_t *cmd)
{
	if(cmd->in != NULL)
		raw_delete(cmd->in);

	if(cmd->out != NULL)
		raw_delete(cmd->out);

	ast_pipe_clear(cmd->pipe);
	free(cmd);
}


/**
 * Create a new pipe. 
 *  @imm: The immediate value.
 *  &returns: The pipe.
 */
struct ast_pipe_t *ast_pipe_new(struct imm_t *imm)
{
	struct ast_pipe_t *pipe;

	pipe = malloc(sizeof(struct ast_pipe_t));
	pipe->imm = imm;
	pipe->next = NULL;

	return pipe;
}

/**
 * Clear a pipe chain.
 *   @pipe: The pipe.
 */
void ast_pipe_clear(struct ast_pipe_t *pipe)
{
	struct ast_pipe_t *tmp;

	while(pipe != NULL) {
		pipe = (tmp = pipe)->next;
		imm_delete(tmp->imm);
		free(tmp);
	}
}
