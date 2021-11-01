#include "inc.h"


/**
 * Evaluate from the top.
 *   @block: The block.
 *   @ctx: The context.
 */
void eval_top(struct block_t *block, struct ctx_t *ctx)
{
	struct env_t env;
	struct stmt_t *stmt;

	env = env_new(NULL);

	for(stmt = block->stmt; stmt != NULL; stmt = stmt->next)
		eval_stmt(stmt, ctx, &env);

	env_delete(env);
}

/**
 * Evaluate a block.
 *   @block: The block.
 *   @ctx: The context.
 */
void eval_block(struct block_t *block, struct ctx_t *ctx, struct env_t *env)
{
	struct stmt_t *stmt;

	for(stmt = block->stmt; stmt != NULL; stmt = stmt->next)
		eval_stmt(stmt, ctx, env);
}

/**
 * Evaluate a statement.
 *   @stmt: The statement.
 *   @ctx: The context.
 *   @env: The environment.
 */
void eval_stmt(struct stmt_t *stmt, struct ctx_t *ctx, struct env_t *env)
{
	switch(stmt->tag) {
	case assign_v: {
		char *id;
		struct val_t *val;
		struct assign_t *assign = stmt->data.assign;

		id = val_id(eval_raw(assign->id, ctx, env), stmt->loc);
		val = eval_imm(assign->val, ctx, env);

		env_put(env, bind_val(id, val));
	} break;

	case syn_v: {
		struct seq_t *seq;
		struct syn_t *syn = stmt->data.syn;
		struct target_list_t *gens, *deps;
		struct val_t *gen, *dep, *iter;
		struct link_t *link;

		seq = seq_new();
		gens = target_list_new();
		deps = target_list_new();

		gen = eval_imm(syn->gen, ctx, env);
		for(iter = gen; iter != NULL; iter = iter->next)
			target_list_add(gens, ctx_target(ctx, iter->spec, iter->str));

		dep = eval_imm(syn->dep, ctx, env);
		for(iter = dep; iter != NULL; iter = iter->next)
			target_list_add(deps, ctx_target(ctx, iter->spec, iter->str));

		ctx->gen = gen;
		ctx->dep = dep;
		for(link = syn->cmd->head; link != NULL; link = link->next)
			seq_add(seq, eval_imm(link->val, ctx, env));

		ctx->gen = ctx->dep = NULL;
		val_clear(gen);
		val_clear(dep);
		ctx_rule(ctx, NULL, gens, deps, seq);
	} break;

	case dir_v: {
		char *str;
		bool cont;
		struct dir_t *dir = stmt->data.dir;

		str = val_str(eval_raw(dir->raw, ctx, env), dir->raw->loc);
		if(ctx->dir == NULL) {
			cont = dir->def;
			if(cont)
				ctx->dir = strdup(str);
		}
		else
			cont = (strcmp(str, ctx->dir) == 0);

		free(str);

		if(cont && (dir->block != NULL))
			eval_block(dir->block, ctx, env);
	} break;

	case print_v: {
		struct val_t *val, *iter;

		val = eval_imm(stmt->data.print->imm, ctx, env);

		for(iter = val; iter != NULL; iter = iter->next)
			print("%s%s", iter->str, iter->next ? " " : "");

		val_clear(val);
	} break;
	}
}

/**
 * Evaluate an immediate value.
 *   @imm: The immediate value.
 *   @ctx: The context.
 *   @env: The environment.
 */
struct val_t *eval_imm(struct imm_t *imm, struct ctx_t *ctx, struct env_t *env)
{
	struct raw_t *raw;
	struct val_t *val = NULL, **iter = &val;

	for(raw = imm->raw; raw != NULL; raw = raw->next) {
		*iter = eval_raw(raw, ctx, env);
		while(*iter != NULL)
			iter = &(*iter)->next;
	}

	return val;
}

/**
 * Evaluate a raw value.
 *   @raw: The raw.
 *   @ctx: The context.
 *   @env: The environment.
 */
struct val_t *eval_raw(struct raw_t *raw, struct ctx_t *ctx, struct env_t *env)
{
	const char *cur, *ptr;
	struct buf_t ret;
	struct val_t *val, *iter;

	ptr = strchr(raw->str, '$');
	if(ptr == NULL)
		return val_new(!raw->quote && (raw->str[0] == '.'), strdup(raw->str));

	if(ptr == raw->str) {
		val = eval_var(&ptr, raw->loc, ctx, env);
		if(*ptr == '\0')
			return val;

		ret = buf_new(32);
	}
	else {
		ret = buf_new(32);
		buf_mem(&ret, raw->str, ptr - raw->str);

		val = eval_var(&ptr, raw->loc, ctx, env);
	}

	for(;;) {
		for(iter = val; iter != NULL; iter = iter->next) {
			buf_str(&ret, iter->str);
			if(iter->next != NULL)
				buf_ch(&ret, ' ');
		}

		val_clear(val);

		ptr = strchr(cur = ptr, '$');
		if(ptr == NULL)
			break;

		val = eval_var(&ptr, raw->loc, ctx, env);
		buf_mem(&ret, cur, ptr - cur);
	}

	buf_str(&ret, cur);

	return val_new(false, buf_done(&ret));
}

/**
 * Evaluate a variable.
 *   @str: Ref. The string.
 *   @loc: The location.
 *   @ctx: The context.
 *   @env: The environment.
 *   &returns: The value.
 */
struct val_t *eval_var(const char **str, struct loc_t loc, struct ctx_t *ctx, struct env_t *env)
{
	char id[256];
	struct bind_t *bind;
	struct val_t *val, *iter;

	if(**str != '$')
		return NULL;

	(*str)++;
	if((*str)[0] == '$')
		return val_new(false, "$$");
	else if((*str)[0] == '{') {
		fatal("FIXME stub");
	}
	else {
		get_var(str, id, loc);

		if(strcmp(id, "@") == 0) {
			if(ctx->gen == NULL)
				loc_err(loc, "Variable `$@` can only be used within a recipe.");

			val = val_dup(ctx->gen);
			for(iter = val; iter != NULL; iter = iter->next)
				str_set(&val->str, str_fmt("$~%s", val->str));
			
			return val;
		}
		else if(strcmp(id, "^") == 0) {
			if(ctx->gen == NULL)
				loc_err(loc, "Variable `$^` can only be used within a recipe.");

			return val_dup(ctx->dep);
		}
		else if(strcmp(id, "~") == 0) 
			return ctx->dir ? val_new(false, strdup(ctx->dir)) : NULL;

		bind = env_get(env, id);
		if(bind == NULL)
			loc_err(loc, "Unknown variable '%s'.", id);

		switch(bind->tag) {
		case val_v: val = val_dup(bind->data.val); break;
		case ns_v: fatal("FIXME stub namespace binding");
		case rule_v: fatal("FIXME stub namespace binding");
		default: __builtin_unreachable();
		}
	}

	return val;
}


/**
 * Create an environment.
 *   @up: The parent environment
 */
struct env_t env_new(struct env_t *up)
{
	return (struct env_t){ map0_new((cmp_f)strcmp, (del_f)bind_delete), up };
}

/**
 * Delete an environment.
 *   @env: The environment.
 */
void env_delete(struct env_t env)
{
	map0_delete(env.map);
}


/**
 * Get a binding from an environment.
 *   @env: The environment.
 *   @id: The identifier.
 *   &returns: The binding if found.
 */
struct bind_t *env_get(struct env_t *env, const char *id)
{
	struct bind_t *bind;

	while(env != NULL) {
		bind = map0_get(env->map, id);
		if(bind != NULL)
			return bind;

		env = env->up;
	}

	return NULL;
}

/**
 * Add a binding to an environment.
 *   @env: The environment.
 *   @bind: The binding.
 */
void env_put(struct env_t *env, struct bind_t *bind)
{
	struct bind_t *cur;

	cur = map_rem(env->map, bind->id);
	if(cur != NULL)
		bind_delete(cur);

	map0_add(env->map, bind->id, bind);
}
