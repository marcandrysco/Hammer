#include "inc.h"


/**
 * Create a job controller.
 *   @queue: The rule queue.
 *   @n: The maximum number of concurrent jobs.
 *   &returns: The controller.
 */
struct ctrl_t *ctrl_new(struct queue_t *queue, uint32_t n)
{
	uint32_t i;
	struct ctrl_t *ctrl;

	ctrl = malloc(sizeof(struct ctrl_t));
	ctrl->queue = queue;
	ctrl->cnt = n;
	ctrl->job = malloc(n * sizeof(struct job_t));

	for(i = 0; i < n; i++)
		ctrl->job[i].pid = -1;

	return ctrl;
}

/**
 * Delete a job controller.
 *   @ctrl: The controller.
 */
void ctrl_delete(struct ctrl_t *ctrl)
{
	free(ctrl->job);
	free(ctrl);
}


/**
 * Add a command to the controller.
 *   @ctrl: The controller.
 *   @rule: The rule.
 */
void ctrl_add(struct ctrl_t *ctrl, struct rule_t *rule)
{
	uint32_t i;

	if(rule->seq->head == NULL)
		return ctrl_done(ctrl, rule);

	for(i = 0; i < ctrl->cnt; i++) {
		if(ctrl->job[i].pid < 0)
			break;
	}

	if(i >= ctrl->cnt)
		fatal("Failed to start job.");

	ctrl->job[i].pid = ctrl_exec(rule->seq->head);
	ctrl->job[i].rule = rule;
	ctrl->job[i].cmd = rule->seq->head->next;
}

/**
 * Determine if there is an available job.
 *   @ctrl: The controller.
 *   &returns: True if available.
 */
bool ctrl_avail(struct ctrl_t *ctrl)
{
	uint32_t i;

	for(i = 0; i < ctrl->cnt; i++) {
		if(ctrl->job[i].pid < 0)
			return true;
	}

	return false;
}

/**
 * Determine if there is at least one busy job.
 *   @ctrl: The controller.
 *   &returns: True if busy.
 */
bool ctrl_busy(struct ctrl_t *ctrl)
{
	uint32_t i;

	for(i = 0; i < ctrl->cnt; i++) {
		if(ctrl->job[i].pid >= 0)
			return true;
	}

	return false;
}

/**
 * Wait for a job to complete.
 *   @ctrl: The controller.
 */
void ctrl_wait(struct ctrl_t *ctrl)
{
	uint32_t i;
	int pid;

	pid = os_wait();

	for(i = 0; i < ctrl->cnt; i++) {
		if(ctrl->job[i].pid == pid)
			break;
	}

	if(i >= ctrl->cnt)
		return;

	if(ctrl->job[i].cmd != NULL) {
		ctrl->job[i].pid = ctrl_exec(ctrl->job[i].cmd);
		ctrl->job[i].cmd = ctrl->job[i].cmd->next;
	}
	else {
		ctrl->job[i].pid = -1;
		ctrl_done(ctrl, ctrl->job[i].rule);
	}
}

/**
 * Done with a rule, adding rules to queue as ready.
 *   @ctrl: The controller.
 *   @rule: The rule.
 */
void ctrl_done(struct ctrl_t *ctrl, struct rule_t *rule)
{
	struct edge_t *edge;
	struct target_t *target;
	struct target_iter_t iter;

	iter = target_iter(rule->gens);
	while((target = target_next(&iter)) != NULL) {
		target->mtime = -1;

		for(edge = target->edge; edge != NULL; edge = edge->next) {
			edge->rule->edges--;
			if(edge->rule->edges == 0)
				queue_add(ctrl->queue, edge->rule);
		}
	}
}


/**
 * Execute a command.
 *   @cmd: The command.
 *   &returns: The PID.
 */
int ctrl_exec(struct cmd_t *cmd)
{
	struct val_t *val;
	struct rt_pipe_t *pipe;

	for(pipe = cmd->pipe; pipe != NULL; pipe = pipe->next) {
		for(val = pipe->cmd; val != NULL; val = val->next)
			print("%s%s", val->str, (val->next != NULL) ? " " : "");

		if(pipe->next != NULL)
			print(" | ");
	}

	if(cmd->in != NULL)
		print(" < %s", cmd->in);

	if(cmd->out != NULL)
		print(" >%s %s", cmd->append ? ">" : "", cmd->out);

	print("\n");

	return os_exec(cmd);
}
