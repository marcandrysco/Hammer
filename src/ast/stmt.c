#include "../inc.h"


/**
 * Create a statement.
 *   @tag: The tag.
 *   @data: Consumed. The data.
 *   @loc: The location.
 *   &returns: The statement.
 */
struct ast_stmt_t *stmt_new(enum stmt_e tag, union stmt_u data, struct loc_t loc)
{
	struct ast_stmt_t *stmt;

	stmt = malloc(sizeof(struct ast_stmt_t));
	stmt->tag = tag;
	stmt->data = data;
	stmt->loc = loc;
	stmt->next = NULL;

	return stmt;
}

/**
 * Delete a statement.
 *   @stmt: The statement.
 */
void stmt_delete(struct ast_stmt_t *stmt)
{
	switch(stmt->tag) {
	case ast_bind_v: ast_bind_delete(stmt->data.bind); break;
	case syn_v: ast_rule_delete(stmt->data.syn); break;
	case loop_v: loop_delete(stmt->data.loop); break;
	case print_v: print_delete(stmt->data.print); break;
	case ast_mkdep_v: ast_mkdep_delete(stmt->data.mkdep); break;
	case block_v: ast_block_delete(stmt->data.block); break;
	case ast_inc_v: ast_inc_delete(stmt->data.inc); break;
	}

	free(stmt);
}

/**
 * Clear a list of statement.
 *   @stmt: The statement list.
 */
void stmt_clear(struct ast_stmt_t *stmt)
{
	struct ast_stmt_t *tmp;

	while(stmt != NULL) {
		stmt = (tmp = stmt)->next;
		stmt_delete(tmp);
	}
}


/**
 * Create a make dependency statement.
 *   @mkdep: The make dependency.
 *   @loc: The location.
 */
struct ast_stmt_t *ast_stmt_mkdep(struct ast_mkdep_t *mkdep, struct loc_t loc)
{
	return stmt_new(ast_mkdep_v, (union stmt_u){ .mkdep = mkdep }, loc);
}

/**
 * Create an include dependency statement.
 *   @inc: The include.
 *   @loc: The location.
 */
struct ast_stmt_t *ast_stmt_inc(struct ast_inc_t *inc, struct loc_t loc)
{
	return stmt_new(ast_inc_v, (union stmt_u){ .inc = inc }, loc);
}


/**
 * Create a make dependency statement.
 *   @path: The path as an immediate.
 *   @loc: The location.
 *   &returns: The make dependency.
 */
struct ast_mkdep_t *ast_mkdep_new(struct imm_t *path, struct loc_t loc)
{
	struct ast_mkdep_t *dep;

	dep = malloc(sizeof(struct ast_mkdep_t));
	*dep = (struct ast_mkdep_t){ path, loc };

	return dep;
}

/**
 * Delete a make dependency statement.
 *   @dep: The make dependency.
 */
void ast_mkdep_delete(struct ast_mkdep_t *dep)
{
	imm_delete(dep->path);
	free(dep);
}

/**
 * Evaluate a make dependency statement.
 *   @dep: The make dependency.
 *   @ctx: The context.
 *   @env: The environment.
 */
void ast_mkdep_eval(struct ast_mkdep_t *dep, struct rt_ctx_t *ctx, struct env_t *env)
{
	struct val_t *val;
	struct rt_obj_t obj;

	obj = eval_imm(dep->path, ctx, env, dep->loc);
	if(obj.tag == rt_null_v)
		return;

	if(obj.tag != rt_val_v)
		loc_err(dep->loc, "Command `makedep` requires a string value.");

	for(val = obj.data.val; val != NULL; val = val->next)
		mk_eval(ctx, val->str, false);

	rt_obj_delete(obj);
}


/**
 * Create an include statement.
 *   @nest: The nest flag.
 *   @opt: The optional flag.
 *   @imm: The immediate value.
 *   &returns: The include.
 */
struct ast_inc_t *ast_inc_new(bool nest, bool opt, struct imm_t *imm)
{
	struct ast_inc_t *inc;

	inc = malloc(sizeof(struct ast_inc_t));
	inc->nest = nest;
	inc->opt = opt;
	inc->imm = imm;

	return inc;
}

/**
 * Delete an include statement.
 *   @inc: The include.
 */
void ast_inc_delete(struct ast_inc_t *inc)
{
	imm_delete(inc->imm);
	free(inc);
}


/**
 * Evaluate an include statement.
 *   @inc: The include.
 *   @ctx: The context.
 *   @env: The environment.
 *   @loc: The statement location.
 */
void ast_inc_eval(struct ast_inc_t *inc, struct rt_ctx_t *ctx, struct env_t *env, struct loc_t loc)
{
	struct val_t *val;
	struct rt_obj_t obj;
	struct env_t *nest;
	struct ast_block_t *top;

	obj = eval_imm(inc->imm, ctx, env, loc);
	if(obj.tag == rt_null_v)
		return;
	else if(obj.tag != rt_val_v)
		loc_err(loc, "%s require string values.", inc->nest ? "Import" : "Include");

	for(val = obj.data.val; val != NULL; val = val->next) {
		top = ham_load(val->str);
		if(top == NULL) {
			if(inc->opt)
				continue;

			loc_err(loc, "Cannot open '%s'.", val->str);
		}

		if(inc->nest) {
			nest = rt_env_new(env);
			eval_block(top, ctx, nest);
			rt_env_delete(nest);
		}
		else
			eval_block(top, ctx, env);

		ast_block_delete(top);
		rt_obj_delete(obj);
	}
}


/**
 * Parse a make dependency file.
 *   @context: The context.
 *   @path: The path.
 *   @strict: The strict flag.
 */
void mk_eval(struct rt_ctx_t *ctx, const char *path, bool strict)
{
	int ch;
	char *str;
	FILE *file;
	struct target_list_t *gens, *deps;

	file = fopen(path, "r");
	if(file == NULL) {
		if(!strict)
			return;

		fatal("Cannot open '%s'.", path);
	}

	gens = target_list_new();
	deps = target_list_new();
	ch = fgetc(file);

	for(;;) {
		while(mk_space(ch) || (ch == '\n'))
			ch = fgetc(file);

		if(ch == EOF)
			break;

		while((str = mk_str(file, &ch)) != NULL) {
			target_list_add(gens, ctx_target(ctx, false, str));
			free(str);
		}

		mk_trim(file, &ch);
		if(ch != ':') {
			fprintf(stderr, "%s: Invalid makedep file.\n", path);

			if(strict)
				exit(1);

			goto clean;
		}

		ch = fgetc(file);
		mk_trim(file, &ch);

		while((str = mk_str(file, &ch)) != NULL) {
			target_list_add(deps, ctx_target(ctx, false, str));
			free(str);
		}

		ctx_rule(ctx, NULL, gens, deps);
		gens = target_list_new();
		deps = target_list_new();
	}

clean:
	target_list_delete(gens);
	target_list_delete(deps);
	fclose(file);
}


/**
 * Trim whitespace.
 *   @file: The file.
 *   @ch: The character reference.
 */
void mk_trim(FILE *file, int *ch)
{
	for(;;) {
		while(mk_space(*ch))
			*ch = fgetc(file);

		if(*ch != '\\')
			break;

		if((*ch = fgetc(file)) != '\n')
			break;

		*ch = fgetc(file);
	}
}

/**
 * Parse a string.
 *   @file: The file.
 *   @ch: The character reference.
 *   &returns: The string, or null if no string.
 */
char *mk_str(FILE *file, int *ch)
{
	struct buf_t buf;

	mk_trim(file, ch);

	if(!mk_ident(*ch))
		return NULL;

	buf = buf_new(32);

	do
		buf_ch(&buf, *ch);
	while(mk_ident(*ch = fgetc(file)));

	return buf_done(&buf);
}


/**
 * Check if a character is a non-newline space.
 *   @ch: The character.
 *   &returns: True if a space.
 */
bool mk_space(int ch)
{
	return (ch == ' ') || (ch == '\t') || (ch == '\r');
}

/**
 * Check if a character is a identifier character..
 *   @ch: The character.
 *   &returns: True if a character.
 */
bool mk_ident(int ch)
{
	return (ch > 0) && !mk_space(ch) && (ch != ':') && (ch != '\n');
}
