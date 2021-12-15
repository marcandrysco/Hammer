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
	env_put(&env, bind_func(strdup(".sub"), fn_sub));
	env_put(&env, bind_func(strdup(".pat"), fn_pat));

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
		struct bind_t *bind;
		struct assign_t *assign = stmt->data.assign;

		id = val_id(eval_raw(assign->id, ctx, env), stmt->loc);
		val = eval_imm(assign->val, ctx, env);

		bind = env_get(env, id);
		if(bind != NULL) {
			if(assign->add) {

				free(id);
			}
			else
				bind_reval(bind, val);
		}
		else
			env_put(env, bind_val(id, val));
	} break;

	case syn_v: {
		struct seq_t *seq;
		struct syn_t *syn = stmt->data.syn;
		struct target_list_t *gens, *deps;
		struct val_t *gen, *dep, *iter;
		struct link_t *link;
		struct ast_cmd_t *proc;

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
		for(link = syn->cmd->head; link != NULL; link = link->next) {
			char *out, *in;

			proc = link->val;
			in = proc->in ? val_str(eval_raw(proc->in, ctx, env), syn->loc) : NULL;
			out = proc->out ? val_str(eval_raw(proc->out, ctx, env), syn->loc) : NULL;

			struct ast_pipe_t *iter;
			struct rt_pipe_t *pipe = NULL, **ipipe = &pipe;

			for(iter = proc->pipe; iter != NULL; iter = iter->next) {
				*ipipe = rt_pipe_new(eval_imm(iter->imm, ctx, env));
				ipipe = &(*ipipe)->next;
			}

			seq_add(seq, pipe, in, out, proc->append);
		}

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
				bind_set(&ctx->dir, bind_val(strdup("~"), val_new(false, strdup(str))));
		}
		else
			cont = (strcmp(str, ctx->dir->data.val->str) == 0);

		free(str);

		if(cont && (dir->block != NULL))
			eval_block(dir->block, ctx, env);
	} break;

	case loop_v: {
		struct env_t nest;
		struct val_t *val, *iter;
		struct loop_t *loop = stmt->data.loop;

		val = eval_imm(loop->imm, ctx, env);

		for(iter = val; iter != NULL; iter = iter->next) {
			nest = env_new(env);
			env_put(&nest, bind_val(strdup(loop->id), val_new(iter->spec, strdup(iter->str))));
			eval_stmt(loop->body, ctx, &nest);
			env_delete(nest);
		}

		val_clear(val);
	} break;

	case print_v: {
		struct val_t *val, *iter;

		val = eval_imm(stmt->data.print->imm, ctx, env);

		for(iter = val; iter != NULL; iter = iter->next)
			print("%s%s", iter->str, iter->next ? " " : "");

		val_clear(val);
	} break;

	case block_v: {
		struct env_t nest;

		nest = env_new(env);
		eval_block(stmt->data.block, ctx, &nest);
		env_delete(nest);
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
 * Expander structure.
 *   @buf: The string buffer.
 *   @orig, str: The origin and active string pointers.
 *   @ctx: The context.
 *   @env: The environment.
 *   @loc: The location.
 */
struct exp_t {
	struct buf_t buf;
	const char *orig, *str;

	struct ctx_t *ctx;
	struct env_t *env;
	struct loc_t loc;
};


/*
 * expansion declarations
 */
struct val_t *exp_get(struct exp_t *exp);

char exp_ch(struct exp_t *exp);
char exp_adv(struct exp_t *exp);
char exp_buf(struct exp_t *exp);
char exp_trim(struct exp_t *exp);

void exp_str(struct exp_t *exp);
void exp_escape(struct exp_t *exp);
void exp_quote1(struct exp_t *exp);
void exp_quote2(struct exp_t *exp);
struct val_t *exp_var(struct exp_t *exp);
struct bind_t *exp_bind(struct exp_t *exp);
void exp_flat(struct exp_t *expr, struct val_t *val);

__attribute__((noreturn)) void exp_err(struct exp_t *exp, const char *fmt, ...);


/**
 * Get an expaneded a value.
 *   @exp: The expander.
 *   &returns: The value.
 */
struct val_t *exp_get(struct exp_t *exp)
{
	struct val_t *val;
	struct buf_t tmp;

	tmp = exp->buf;
	exp->buf = buf_new(32);

	if(*exp->str == '$') {
		val = exp_var(exp);
		if(*exp->str != '\0') {
			exp_flat(exp, val);
			exp_str(exp);
			val = val_new(false, strdup(buf_done(&exp->buf)));
		}
	}
	else if(*exp->str == '.') {
		exp_buf(exp);
		while(ch_var(exp_ch(exp)))
			exp_buf(exp);

		if(*exp->str != '\0') {
			exp_str(exp);
			val = val_new(false, strdup(buf_done(&exp->buf)));
		}
		else
			val = val_new(true, strdup(buf_done(&exp->buf)));
	}
	else {
		exp_str(exp);
		val = val_new(false, strdup(buf_done(&exp->buf)));
	}

	buf_delete(&exp->buf);
	exp->buf = tmp;

	return val;
}


/**
 * Advance the expander a character.
 *   @exp: The expander.
 *   &returns: The next character.
 */
char exp_ch(struct exp_t *exp)
{
	return *exp->str;
}

/**
 * Advance the expander a character.
 *   @exp: The expander.
 *   &returns: The next character.
 */
char exp_adv(struct exp_t *exp)
{
	exp->str++;

	return *exp->str;
}

/**
 * Buffer the current character on an expander.
 *   @exp: The expander.
 *   &returns: The next character.
 */
char exp_buf(struct exp_t *exp)
{
	buf_ch(&exp->buf, *exp->str);

	return exp_adv(exp);
}

/**
 * Trim whitespace from an expander.
 *   @exp: The expander.
 *   &returns: The next character.
 */
char exp_trim(struct exp_t *exp)
{
	const char *str;

	str = exp->str;
	while((*str == ' ') || (*str == '\t') || (*str == '\n'))
		str++;

	exp->str = str;
	return *str;
}


/**
 * Expand a string.
 *   @exp: The expander.
 */
void exp_str(struct exp_t *exp)
{
	char ch;

	for(;;) {
		ch = exp_ch(exp);
		if(ch == '\0')
			break;
		else if(ch == '\\')
			exp_escape(exp);
		else if(ch == '\'')
			exp_quote1(exp);
		else if(ch == '"')
			exp_quote2(exp);
		else if(ch == '$')
			exp_flat(exp, exp_var(exp));
		else if(ch_str(ch))
			exp_buf(exp);
		else
			break;
	}
}

/**
 * Expand an escape sequence.
 *   @exp: The expander.
 */
void exp_escape(struct exp_t *exp)
{
	char ch;

	switch(exp_adv(exp)) {
	case '\\': ch = '\\'; break;
	case '\'': ch = '\''; break;
	case '\"': ch = '\"'; break;
	case 't': ch = '\t'; break;
	case 'n': ch = '\n'; break;
	case '$': ch = '$'; break;
	case ' ': ch = ' '; break;
	default: exp_err(exp, "Invalid escape sequence '\\%c'.", exp_ch(exp));
	}

	buf_ch(&exp->buf, ch);
	exp_adv(exp);
}

/**
 * Expand a single-quoted string.
 *   @exp: The expander.
 */
void exp_quote1(struct exp_t *exp)
{
	char ch;

	exp_adv(exp);
	
	for(;;) {
		ch = exp_ch(exp);
		if(ch == '\'')
			break;
		else if(ch == '\\')
			exp_escape(exp);
		else
			exp_buf(exp);
	}

	exp_adv(exp);
}

/**
 * Expand a double-quoted string.
 *   @exp: The expander.
 */
void exp_quote2(struct exp_t *exp)
{
	char ch;

	exp_adv(exp);
	
	for(;;) {
		ch = exp_ch(exp);
		if(ch == '"')
			break;
		else if(ch == '\\')
			exp_escape(exp);
		else if(ch == '$')
			exp_flat(exp, exp_var(exp));
		else
			exp_buf(exp);
	}

	exp_adv(exp);
}


/**
 * Expand a variable.
 *   @exp: The expander.
 *   &returns: The value.
 */
struct val_t *exp_var(struct exp_t *exp)
{
	char ch;
	struct val_t *val;
	struct bind_t *bind;

	ch = exp_adv(exp);
	if(ch == '{') {
		exp_adv(exp);
		bind = exp_bind(exp);

		if(bind->tag == func_v)
			exp_err(exp, "Function '%s' used as a value.", bind->id);

		if(bind->tag == ns_v) {
			fatal("FIXME stub ns");
		}

		val = val_dup(bind->data.val);
		while((ch = exp_trim(exp)) != '}') {
			uint32_t cnt;
			const char *id;
			struct val_t **args;
			struct buf_t buf;

			if(ch != '.')
				exp_err(exp, "Expected '.' or '}'.");

			buf = buf_new(32);
			buf_ch(&buf, ch);

			exp_adv(exp);
			ch = exp_trim(exp);
			if(!ch_var(ch))
				exp_err(exp, "Expected function name.");

			do
				buf_ch(&buf, ch);
			while(ch_var(ch = exp_adv(exp)));

			id = buf_done(&buf);
			bind = env_get(exp->env, id);
			if(bind == NULL)
				exp_err(exp, "Unknown function '%s'.", id);

			args_init(&args, &cnt);
			args_add(&args, &cnt, val);

			if(exp_trim(exp) != '(')
				exp_err(exp, "Expected '('.");

			exp_adv(exp);
			if(exp_trim(exp) != ')') {
				for(;;) {
					args_add(&args, &cnt, exp_get(exp));
					ch = exp_trim(exp);
					if(ch == ')')
						break;
					else if(ch != ',')
						exp_err(exp, "Expected ',' or '('.");

					exp_adv(exp);
					exp_trim(exp);
				}
			}

			exp_adv(exp);

			if(bind->tag == val_v)
				exp_err(exp, "Variable '%s' used as a function.", id);
			else if(bind->tag == ns_v)
				exp_err(exp, "Namespace '%s' used as a function.", id);

			val = bind->data.func(args, cnt, exp->loc);

			args_delete(args, cnt);
			buf_delete(&buf);
		}

		exp_adv(exp);
	}
	else {
		bind = exp_bind(exp);

		switch(bind->tag) {
		case val_v: val = val_dup(bind->data.val); break;
		case func_v: exp_err(exp, "Cannot use function as a value.");
		case ns_v: exp_err(exp, "Cannot use namspace as a value.");
		default: fatal("Unreachable.");
		}
	}

	return val;
}

/**
 * Flatten a value.
 *   @exp: The expander.
 *   @val: The value.
 */
void exp_flat(struct exp_t *exp, struct val_t *val)
{
	struct val_t *orig = val;

	while(val != NULL) {
		buf_str(&exp->buf, val->str);

		if(val->next != NULL)
			buf_ch(&exp->buf, ' ');

		val = val->next;
	}

	val_clear(orig);
}

/**
 * Retrieve a variable binding.
 *   @exp: The expander.
 *   &returns: The value.
 */
struct bind_t *exp_bind(struct exp_t *exp)
{
	char *id;
	const char *str;
	struct buf_t buf;
	struct bind_t *bind;

	if(*exp->str == '~') {
		exp_adv(exp);
		return exp->ctx->dir;
	}

	str = exp->str;

	buf = buf_new(32);

	do
		buf_ch(&buf, *str++);
	while(ch_var(*str));

	id = buf_done(&buf);
	bind = env_get(exp->env, id);
	if(bind == NULL)
		exp_err(exp, "Unknown variable '%s'.", id);

	exp->str = str;
	free(id);

	return bind;
}


/**
 * Display an error for string expansion.
 *   @exp: The string expansion.
 *   @fmt: The format string.
 *   @...: The arguments.
 */
void exp_err(struct exp_t *exp, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	fprintf(stderr, "%s:%u:%lu: ", exp->loc.path, exp->loc.lin, exp->loc.col + (exp->str - exp->orig));
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);

	exit(1);
}


/**
 * Evaluate a raw value.
 *   @raw: The raw.
 *   @ctx: The context.
 *   @env: The environment.
 */
struct val_t *eval_raw(struct raw_t *raw, struct ctx_t *ctx, struct env_t *env)
{
	struct exp_t exp;

	exp.orig = exp.str = raw->str;
	exp.loc = raw->loc;
	exp.ctx = ctx;
	exp.env = env;

	return exp_get(&exp);
}


/**
 * Initialize arguments.
 *   @args: The arguments reference.
 *   @cnt: The number of arguments reference.
 */
void args_init(struct val_t ***args, uint32_t *cnt)
{
	*args = malloc(0);
	*cnt = 0;
}

/**
 * Add to arguments.
 *   @args: The arguments reference.
 *   @cnt: The number of arguments reference.
 *   @val: Consumed. The value.
 */
void args_add(struct val_t ***args, uint32_t *cnt, struct val_t *val)
{
	*args = realloc(*args, (*cnt + 1) * sizeof(struct val_t));
	(*args)[(*cnt)++] = val;
}

/**
 * Delete arguments.
 *   @args: The arguments.
 *   @cnt: The number of arguments.
 */
void args_delete(struct val_t **args, uint32_t cnt)
{
	uint32_t i;

	for(i = 0; i < cnt; i++)
		val_delete(args[i]);

	free(args);
}


struct val_t *fn_sub(struct val_t **args, uint32_t cnt, struct loc_t loc)
{
	struct buf_t buf;
	struct val_t *val, *ret, **iter;
	const char *str, *find;

	if(cnt != 3)
		loc_err(loc, "Function `.sub` requires 2 arguments.");

	iter = &ret;

	for(val = args[0]; val != NULL; val = val->next) {
		str = val->str;
		buf = buf_new(strlen(val->str) + 1);

		find = strstr(str, args[1]->str);
		while(find != NULL) {
			buf_mem(&buf, str, find - str);
			buf_str(&buf, args[2]->str);

			str = find + strlen(args[1]->str);
			find = strstr(str, args[1]->str);
		}
		buf_str(&buf, str);

		*iter = val_new(val->spec, buf_done(&buf));
		iter = &(*iter)->next;
	}

	*iter = NULL;

	return ret;
}

/**
 * Compute the pre and post lengths of a pattern.
 *   @str: The string.
 *   @pre: Out. The pre-pattern length.
 *   @post: Out The post-pattern length.
 */
bool pat_pre(const char *str, uint32_t *pre, uint32_t *post)
{
	const char *find;

	find = strchr(str, '%');
	if((find == NULL) || (strrchr(str, '%') != find))
		return false;

	*pre = find - str;
	*post = strlen(str) - *pre - 1;
	return true;
}

struct val_t *fn_pat(struct val_t **args, uint32_t cnt, struct loc_t loc)
{
	uint32_t pre, post;
	struct buf_t buf;
	struct val_t *val, *ret, **iter;
	char *pat, *repl;

	if(cnt != 3)
		loc_err(loc, "Function `.pat` requires 2 arguments.");

	iter = &ret;
	pat = val_str(args[1], loc);
	repl = val_str(args[2], loc);

	if(!pat_pre(pat, &pre, &post))
		loc_err(loc, "Function `.pat` requires patterns as arguments (must contain a single '%').");
	//find = strchr(pat, '%');
	//if(find == NULL)
		//loc_err(loc, "Function `.pat` requires patterns as arguments (must contain a single '%').");

	for(val = args[0]; val != NULL; val = val->next) {
		pat_pre(val->str, &pre, &post);
		//for(i = 0; pat[i] != '%'; i++) {
			//if(pat[i] == '\0')
		//}

		buf_new(32);
		buf_done(&buf);
		
		//str = find = NULL;
		//if(find || str);
	}

	free(pat);
	free(repl);
	*iter = NULL;

	return ret;
}

struct val_t *eval_str(const char **str, struct loc_t loc, struct ctx_t *ctx, struct env_t *env)
{
	switch(**str) {
	case '$':
		(*str)++;
		return eval_var(str, loc_off(loc, 1), ctx, env);

	case '"':
	case '\'':
		fatal("FIXME eval_str");

	default: {
		struct buf_t buf;

		if(!ch_str(**str))
			loc_err(loc, "Expected string.");

		buf = buf_new(32);

		do
			buf_ch(&buf, *(*str)++);
		while(ch_str(**str));

		return val_new(false, buf_done(&buf));
	} break;
	}
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
	const char *orig = *str;

	if((*str)[0] == '$')
		return val_new(false, "$$");
	else if((*str)[0] == '{') {
		(*str)++;
		val = eval_var(str, loc_off(loc, *str - orig), ctx, env);
		while(**str != '}') {
			uint32_t off, cnt;
			struct val_t **args;

			if(**str != '.')
				loc_err(loc_off(loc, *str - orig), "Expected '.' or '}'.");

			off = *str - orig;
			(*str)++;
			id[0] = '.';
			get_var(str, id + 1, loc);

			bind = env_get(env, id);
			if(bind == NULL)
				loc_err(loc_off(loc, off), "Unknown function '%s'.", id);

			args_init(&args, &cnt);
			args_add(&args, &cnt, val);

			str_trim(str);
			if(**str != '(')
				loc_err(loc_off(loc, *str - orig), "Expected '.' or '}'.");

			(*str)++;
			str_trim(str);
			if(**str != ')') {
				for(;;) {
					args_add(&args, &cnt, eval_str(str, loc_off(loc, *str - orig), ctx, env));
					str_trim(str);
					if(**str == ')')
						break;
					else if(**str != ',')
						loc_err(loc_off(loc, *str - orig), "Expected ',' or ')'.");

					(*str)++;
					str_trim(str);
				}
			}
			(*str)++;

			switch(bind->tag) {
			case val_v:
				loc_err(loc_off(loc, off), "Cannot call a value.");

			case func_v:
				val = bind->data.func(args, cnt, loc_off(loc, off));
				break;

			case ns_v:
				loc_err(loc_off(loc, off), "Cannot call a namespace.");
			}

			args_delete(args, cnt);
		}
		(*str)++;

		return val;
	}
	else {
		get_var(str, id, loc_off(loc, *str - orig));

		if(strcmp(id, "@") == 0) {
			if(ctx->gen == NULL)
				loc_err(loc_off(loc, *str - orig), "Variable `$@` can only be used within a recipe.");

			val = val_dup(ctx->gen);
			for(iter = val; iter != NULL; iter = iter->next)
				str_set(&val->str, str_fmt("$~%s", val->str));
			
			return val;
		}
		else if(strcmp(id, "^") == 0) {
			if(ctx->gen == NULL)
				loc_err(loc_off(loc, *str - orig), "Variable `$^` can only be used within a recipe.");

			return val_dup(ctx->dep);
		}
		else if(strcmp(id, "~") == 0)
			fatal("STUBME");// return ctx->dir ? val_new(false, strdup(ctx->dir)) : NULL;

		bind = env_get(env, id);
		if(bind == NULL)
			loc_err(loc, "Unknown variable '%s'.", id);

		switch(bind->tag) {
		case val_v: val = val_dup(bind->data.val); break;
		case ns_v: fatal("FIXME stub namespace binding");
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
