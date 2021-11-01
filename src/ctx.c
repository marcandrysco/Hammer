#include "inc.h"


/**
 * Create a new context.
 *   @opt: The options structure.
 *   &returns: The context.
 */
struct ctx_t *ctx_new(const struct opt_t *opt)
{
	struct ctx_t *ctx;

	ctx = malloc(sizeof(struct ctx_t));
	ctx->opt = opt;
	ctx->map = map_new();
	ctx->rules = rule_list_new();
	ctx->str = malloc(64);
	ctx->dir = opt->dir ? strdup(opt->dir) : NULL;
	ctx->len = 0;
	ctx->max = 64;
	ctx->gens = ctx->deps = NULL;
	ctx->gen = ctx->dep = NULL;

	return ctx;
}

/**
 * Delete the context.
 *   @ctx: The context.
 */
void ctx_delete(struct ctx_t *ctx)
{
	if(ctx->dir != NULL)
		free(ctx->dir);

	map_delete(ctx->map);
	rule_list_delete(ctx->rules);
	free(ctx->str);
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
	struct rule_iter_t irule;
	struct queue_t *queue;

	queue = queue_new();

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

	if(ctx->dir == NULL)
		ctx->dir = strdup("");
	else if(ctx->dir[0] == '\0')
		str_set(&ctx->dir, strdup("./"));
	else if(ctx->dir[strlen(ctx->dir) - 1] != '/')
		str_set(&ctx->dir, str_fmt("%s/", ctx->dir));

	struct ent_t *ent; //FIXME uhg

	for(ent = ctx->map->ent; ent != NULL; ent = ent->next) {
		struct target_t *target = ent->target;

		if(target->flags & FLAG_BUILD)
			str_set(&target->path, str_fmt("%s%s", ctx->dir, target->path));
	}

	irule = rule_iter(ctx->rules);
	while((rule = rule_next(&irule)) != NULL) {
		struct cmd_t *cmd;

		for(cmd = rule->seq->head; cmd != NULL; cmd = cmd->next)
			val_final(cmd->val, ctx->dir);
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

		if((max > min) || ctx->opt->force) {
			iter = target_iter(rule->gens);
			while((target = target_next(&iter)) != NULL) {
				char *path, *iter;

				if(target->flags & FLAG_SPEC)
					continue;

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


/**
 * Add a character to the context string.
 *   @ctx: The context.
 *   @ch: The character.
 */
void ctx_ch(struct ctx_t *ctx, char ch)
{
	if(ctx->len == ctx->max)
		ctx->str = realloc(ctx->str, ctx->max *= 2);

	ctx->str[ctx->len++] = ch;
}

void ctx_bufstr(struct ctx_t *ctx, const char *buf)
{
	while(*buf != '\0')
		ctx_ch(ctx, *buf++);
}

void ctx_buf(struct ctx_t *ctx, const char *buf, uint32_t len)
{
	while(len-- > 0)
		ctx_ch(ctx, *buf++);
}

struct val_t *ctx_var(struct ctx_t *ctx, struct ns_t *ns, const char *id, struct loc_t loc)
{
	struct bind_t *bind;

	if(strcmp(id, "dir") == 0)
		return val_new(false, ctx->dir);

	bind = ns_find(ns, id);
	if(bind == NULL)
		loc_err(loc, "Unknown variable '%s'.", id);

	if(bind->tag != val_v)
		loc_err(loc, "Name '%s' does not refer to a variable.", id);

	return val_dup(bind->data.val);
}


/**
 * Process a string token, replacing variables as needed.
 *   @ctx: The context.
 *   @ns: The namespace.
 *   @tok: The token.
 *   &returns: The processed string.
 */
const char *ctx_str(struct ctx_t *ctx, struct ns_t *ns, struct tok_t *tok)
{
	const char *ptr, *find, *str = tok->str;

	ctx->len = 0;

	find = (ns ? strchr(str, '$') : NULL);
	while(find != NULL) {
		ctx_buf(ctx, str, find - str);
		if(find[1] == '$') {
			str = find + 2;
			ctx_ch(ctx, '$');
		}
		else if(find[1] == '@') {
			struct target_t *target;
			struct target_iter_t iter;

			if(ctx->gens == NULL)
				loc_err(tok->loc, "Special variable '$@' can only be used in commands.");

			iter = target_iter(ctx->gens);
			while((target = target_next(&iter)) != NULL) {
				if(ctx->dir != NULL) {
					ctx_buf(ctx, ctx->dir, strlen(ctx->dir));
					ctx_ch(ctx, '/');
				}

				ctx_bufstr(ctx, target->path);
			}

			str = find + 2;
		}
		else if(find[1] == '^') {
			struct target_t *target;
			struct target_iter_t iter;

			if(ctx->deps == NULL)
				loc_err(tok->loc, "Special variable '$^' can only be used in commands.");

			iter = target_iter(ctx->deps);
			while((target = target_next(&iter)) != NULL)
				ctx_bufstr(ctx, target->path);

			str = find + 2;
		}
		else if(find[1] == '{') {
			char id[256];
			struct val_t *val, *iter;

			find += 2;
			get_var(&find, id, tok->loc);
			val = ctx_var(ctx, ns, id, tok->loc);

			while(*find == '.') {
				struct val_t **args;
				uint32_t cnt;

				args = malloc(0);
				cnt = 0;

				find++;
				get_var(&find, id, tok->loc);
				if(*find != '(')
					loc_err(tok->loc, "Expected '('.");

				find++;
				if(*find != ')') {
					for(;;) {
						args = realloc(args, (cnt + 1) * sizeof(void *));
						//get_str(&find, id, tok->loc);
						args[cnt++] = ctx_var(ctx, ns, id, tok->loc);

						if(*find == ')')
							break;
						else if(*find != ',')
							loc_err(tok->loc, "Expected ',' or '('.");
						
						find++;
					}
				}

				find++;
				free(args);
				fatal("stub function[%s]", id);
			}

			if(*find != '}')
				loc_err(tok->loc, "Expected '.' or '{'.");

			find++;

			for(iter = val; iter != NULL; iter = iter->next) {
				if(iter != val)
					ctx_ch(ctx, ' ');

				ctx_buf(ctx, iter->str, strlen(iter->str));
			}

			val_delete(val);
			str = find;
		}
		else if(ch_alpha(find[1])) {
			uint32_t i = 0;
			char id[256];
			struct bind_t *bind;

			ptr = find + 1;

			do {
				if(i == 254)
					loc_err(tok->loc, "Variable name too long.");

				id[i++] = *ptr++;
			} while(ch_alnum(*ptr));

			id[i] = '\0';

			if(strcmp(id, "dir") == 0) {
				if(ctx->dir != NULL)
					ctx_buf(ctx, ctx->dir, strlen(ctx->dir));
			}
			else {
				bind = ns_find(ns, id);
				if(bind == NULL)
					loc_err(tok->loc, "Unknown variable '%s'.", id);

				switch(bind->tag) {
				case val_v: {
					struct val_t *val;

					for(val = bind->data.val; val != NULL; val = val->next) {
						if(val != bind->data.val)
							ctx_ch(ctx, ' ');

						ctx_buf(ctx, val->str, strlen(val->str));
					}
				} break;

				case rule_v:
					fatal("FIXME stub");

				case ns_v:
					loc_err(tok->loc, "Cannot use namespace '%s' as a string.", id);
				}
			}

			str = ptr;
		}
		else
			loc_err(tok->loc, "Invalid variable in string.");

		find = strchr(str, '$');
	}

	ctx_buf(ctx, str, strlen(str));
	ctx_ch(ctx, '\0');

	return ctx->str;
}
