#include "inc.h"


/**
 * Create a command sequence.
 *   &returns: The sequence.
 */
struct seq_t *seq_new(void)
{
	struct seq_t *seq;

	seq = malloc(sizeof(struct seq_t));
	*seq = (struct seq_t){ NULL, &seq->head };

	return seq;
}

/**
 * Delete a sequence.
 *   @seq: The sequence.
 */
void seq_delete(struct seq_t *seq)
{
	struct cmd_t *cmd, *tmp;

	cmd = seq->head;
	while(cmd != NULL) {
		cmd = (tmp = cmd)->next;
		rt_pipe_clear(tmp->pipe);

		if(tmp->in != NULL)
			free(tmp->in);

		if(tmp->out != NULL)
			free(tmp->out);

		free(tmp);
	}

	free(seq);
}


/**
 * Add a command to a sequence.
 *   @seq: The sequence.
 *   @pipe: The pipe sequence.
 *   @in: Optional. The input path.
 *   @out: Optional. The output path.
 *   @append: The append flag.
 */
void seq_add(struct seq_t *seq, struct rt_pipe_t *pipe, char *in, char *out, bool append)
{
	struct cmd_t *cmd;

	cmd = malloc(sizeof(struct cmd_t));
	*cmd = (struct cmd_t){ pipe, in, out, append, NULL };

	*seq->tail = cmd;
	seq->tail = &cmd->next;
}


/**
 * Create a pipe.
 *   @cmd: Consumed. The pipe.
 *   &returns: The pipe.
 */
struct rt_pipe_t *rt_pipe_new(struct val_t *cmd)
{
	struct rt_pipe_t *pipe;

	pipe = malloc(sizeof(struct rt_pipe_t));
	pipe->cmd = cmd;
	pipe->next = NULL;

	return pipe;
}

/**
 * Clear a list of pipes.
 *   @pipe: The pipe list.
 */
void rt_pipe_clear(struct rt_pipe_t *pipe)
{
	struct rt_pipe_t *tmp;

	while(pipe != NULL) {
		pipe = (tmp = pipe)->next;
		val_clear(tmp->cmd);
		free(tmp);
	}
}
