#include "inc.h"


/**
 * Create a new context.
 *   @opt: The options structure.
 *   &returns: The context.
 */
struct rt_ctx_t *ctx_new(const struct opt_t *opt)
{
	struct rt_ctx_t *ctx;

	ctx = malloc(sizeof(struct rt_ctx_t));
	ctx->opt = opt;
	ctx->map = map_new();
	ctx->rules = rule_list_new();
	ctx->cur = NULL;
	ctx->gens = ctx->deps = NULL;

	return ctx;
}

/**
 * Delete the context.
 *   @ctx: The context.
 */
void ctx_delete(struct rt_ctx_t *ctx)
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
void ctx_run(struct rt_ctx_t *ctx, const char **builds)
{
	struct ctrl_t *ctrl;
	struct rule_t *rule;
	struct rule_iter_t irule;
	struct queue_t *queue;

	queue = queue_new();
	ctrl = ctrl_new(queue, 4);

	irule = rule_iter(ctx->rules);
	while((rule = rule_next(&irule)) != NULL) {
		uint32_t i;
		struct target_t *target;
		struct target_iter_t iter;

		iter = target_iter(rule->gens);
		while((target = target_next(&iter)) != NULL) {
			for(i = 0; builds[i] != NULL; i++) {
				if(strcmp(target->path, builds[i]) == 0)
					queue_recur(queue, rule);
			}

			if((target->flags & (FLAG_BUILD | FLAG_SPEC)) == 0)
				target->flags |= FLAG_BUILD;
		}
	}

	for(;;) {
		while(!ctrl_avail(ctrl))
			ctrl_wait(ctrl);

		rule = queue_rem(queue);
		if(rule == NULL) {
			if(!ctrl_busy(ctrl))
				break;

			ctrl_wait(ctrl);
			continue;
		}

		struct target_t *target;
		struct target_iter_t iter;
		int64_t min = INT64_MAX - 1, max = INT64_MIN + 1;

		iter = target_iter(rule->gens);
		while((target = target_next(&iter)) != NULL) {
			if(target->flags & FLAG_SPEC)
				min = INT64_MIN, max = INT64_MAX;

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

		iter = target_iter(rule->gens);
		if((max > min) || ctx->opt->force) {
			iter = target_iter(rule->gens);
			while((target = target_next(&iter)) != NULL) {
				char *path, *iter;

				if(target->flags & FLAG_SPEC)
					continue;

				//FIXME parent directory option
				path = strdup(target->path);
				iter = path;
				while((iter = strchr(iter, '/')) != NULL) {
					*iter = '\0';
					os_mkdir(path);
					*iter = '/';
					iter++;
				}

				free(path);
			}

			ctrl_add(ctrl, rule);
		}
		else
			ctrl_done(ctrl, rule);
	}

	while(ctrl_busy(ctrl))
		ctrl_wait(ctrl);

	queue_delete(queue);
	ctrl_delete(ctrl);
}


/**
 * Retrieve a target, creating it if required.
 *   @ctx: The context.
 *   @spec: The special flag.
 *   @path: The path.
 *   &returns: The target.
 */
struct target_t *ctx_target(struct rt_ctx_t *ctx, bool spec, const char *path)
{
	struct target_t *target;

	target = map_get(ctx->map, spec, path);
	if(target == NULL) {
		target = rt_ref_new(spec, strdup(path));
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
 */
struct rule_t *ctx_rule(struct rt_ctx_t *ctx, const char *id, struct target_list_t *gens, struct target_list_t *deps)
{
	struct rule_t *rule;

	if(gens->inst == NULL)
		fatal("FIXME location All rules must have at least one targets.");

	if(id != NULL) {
		fatal("FIXME stub rule w/ id");
	}
	else {
		struct target_t *target;
		struct target_inst_t **inst;
		struct target_iter_t iter;

		iter = target_iter(gens);
		rule = target_next(&iter)->rule;
		if(rule != NULL) {
			uint32_t len;

			if(rule->seq != NULL)
				fatal("Cannot add targets to a rule with a recipe.");

			len = target_list_len(gens);
			if(target_list_len(rule->gens) != len)
				fatal("Partial rules must have matching target lists.");

			iter = target_iter(gens);
			while((target = target_next(&iter)) != NULL) {
				if(!target_list_contains(rule->gens, target))
					fatal("Partial rules must have matching target lists.");
			}

			inst = &rule->deps->inst;
			while(*inst != NULL)
				inst = &(*inst)->next;

			*inst = deps->inst;
			deps->inst = NULL;

			iter = target_iter(deps);
			while((target = target_next(&iter)) != NULL)
				target_conn(target, rule);

			target_list_delete(gens);
			target_list_delete(deps);
		}
		else {
			rule = rule_new(id ? strdup(id) : NULL, gens, deps, NULL);
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
	}

	return rule;
}
