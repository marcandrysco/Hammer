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
		val_clear(tmp->val);
		free(tmp);
	}

	free(seq);
}


/**
 * Add a command to a sequence.
 *   @seq: The sequence.
 *   @val: The value list.
 */
void seq_add(struct seq_t *seq, struct val_t *val)
{
	struct cmd_t *cmd;

	cmd = malloc(sizeof(struct cmd_t));
	*cmd = (struct cmd_t){ val, NULL };

	*seq->tail = cmd;
	seq->tail = &cmd->next;
}
