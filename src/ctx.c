#include "inc.h"


/**
 * Create a new context.
 *   &returns: The context.
 */
struct ctx_t *ctx_new(void)
{
	struct ctx_t *ctx;

	ctx = malloc(sizeof(struct ctx_t));
	ctx->map = map_new();
	ctx->rules = rule_list_new();

	return ctx;
}

/**
 * Delete the context.
 *   @ctx: The context.
 */
void ctx_delete(struct ctx_t *ctx)
{
	map_delete(ctx->map);
	rule_list_delete(ctx->rules);
	free(ctx);
}


/**
 * Run all outdated rules on the context.
 *   @ctx: The context.
 *   @builds: The set of target to build.
 */
void ctx_run(struct ctx_t *ctx, const char **builds)
{
	struct rule_t *rule;
	struct rule_iter_t iter;
	struct queue_t *queue;

	queue = queue_new();

	iter = rule_iter(ctx->rules);
	while((rule = rule_next(&iter)) != NULL) {
		uint32_t i;
		struct target_t *target;
		struct target_iter_t iter;

		iter = target_iter(rule->gens);
		while((target = target_next(&iter)) != NULL) {
			for(i = 0; builds[i] != NULL; i++) {
				if(strcmp(target->path, builds[i]) == 0)
					queue_recur(queue, rule);
			}
		}
	}

	while((rule = queue_rem(queue)) != NULL) {
		struct cmd_t *cmd;
		struct edge_t *edge;
		struct target_t *target;
		struct target_iter_t iter;
		int64_t min = INT64_MAX - 1, max = INT64_MIN + 1;

		iter = target_iter(rule->gens);
		while((target = target_next(&iter)) != NULL) {
			if(target->flags & FLAG_SPEC)
				min = INT64_MIN;

			if(target_mtime(target) < min)
				min = target_mtime(target);
		}

		iter = target_iter(rule->deps);
		while((target = target_next(&iter)) != NULL) {
			if(target->flags & FLAG_SPEC)
				continue;

			if(target_mtime(target) > max)
				max = target_mtime(target);
		}

		if(max > min) {
			for(cmd = rule->seq->head; cmd != NULL; cmd = cmd->next)
				os_exec(cmd->val);
		}

		iter = target_iter(rule->gens);
		while((target = target_next(&iter)) != NULL) {
			for(edge = target->edge; edge != NULL; edge = edge->next) {
				edge->rule->edges--;
				if(edge->rule->edges == 0)
					queue_add(queue, edge->rule);
			}
		}
	}

	queue_delete(queue);
}


/**
 * Retrieve a target, creating it if required.
 *   @ctx: The context.
 *   @spec: The special flag.
 *   @path: The path.
 *   &returns: The target.
 */
struct target_t *ctx_target(struct ctx_t *ctx, bool spec, const char *path)
{
	struct target_t *target;

	target = map_get(ctx->map, spec, path);
	if(target == NULL) {
		target = target_new(spec, strdup(path));
		map_add(ctx->map, target);
	}

	return target;
}

/**
 * Retrieve a rule, creating it if required.
 *   @ctx: The context.
 *   @id: Optional. The rule identifier.
 *   @gens: Consumed. The set of generated targets.
 *   @deps: Consumed. The set of dependency targets.
 *   @seq: Consumed. The command sequence.
 */
struct rule_t *ctx_rule(struct ctx_t *ctx, const char *id, struct target_list_t *gens, struct target_list_t *deps, struct seq_t *seq)
{
	struct rule_t *rule;

	if(id != NULL) {
		fatal("FIXME stub rule w/ id");
	}
	else {
		struct target_t *target;
		struct target_iter_t iter;

		rule = rule_new(id ? strdup(id) : NULL, gens, deps, seq);
		rule_list_add(ctx->rules, rule);

		iter = target_iter(gens);
		while((target = target_next(&iter)) != NULL) {
			if(target->rule != NULL)
				fatal("FIXME target already had rule, better error");

			target->rule = rule;
		}

		iter = target_iter(deps);
		while((target = target_next(&iter)) != NULL)
			target_conn(target, rule);
	}

	return NULL;
}
