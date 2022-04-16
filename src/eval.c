#include "inc.h"


/**
 * Evaluate from the top.
 *   @block: The block.
 *   @ctx: The context.
 */
void eval_top(struct ast_block_t *block, struct rt_ctx_t *ctx)
{
	struct env_t *env;
	struct ast_stmt_t *stmt;

	env = rt_env_new(NULL);
	env_put(env, bind_new(strdup(".sub"), rt_obj_func(fn_sub), (struct loc_t){ }));
	env_put(env, bind_new(strdup(".pat"), rt_obj_func(fn_pat), (struct loc_t){ }));

	for(stmt = block->stmt; stmt != NULL; stmt = stmt->next)
		eval_stmt(stmt, ctx, env);

	rt_env_delete(env);
}

/**
 * Evaluate a block.
 *   @block: The block.
 *   @ctx: The context.
 */
void eval_block(struct ast_block_t *block, struct rt_ctx_t *ctx, struct env_t *env)
{
	struct ast_stmt_t *stmt;

	for(stmt = block->stmt; stmt != NULL; stmt = stmt->next)
		eval_stmt(stmt, ctx, env);
}

/**
 * Evaluate a statement.
 *   @stmt: The statement.
 *   @ctx: The context.
 *   @env: The environment.
 */
void eval_stmt(struct ast_stmt_t *stmt, struct rt_ctx_t *ctx, struct env_t *env)
{
	switch(stmt->tag) {
	case ast_bind_v: {
		char *id;
		struct bind_t *get = NULL;
		struct rt_obj_t obj;
		struct ast_bind_t *bind = stmt->data.bind;

		id = rt_eval_str(bind->id, ctx, env, stmt->loc);

		switch(bind->tag) {
		case ast_val_v:
			obj = eval_imm(bind->data.val, ctx, env, stmt->loc);
			break;

		case ast_func_v:
			fatal("FIXME stub");
			break;

		case ast_block_v: {
			struct env_t *nest;

			nest = rt_env_new(env);
			eval_block(bind->data.block, ctx, nest);
			nest->next = NULL;
			obj = rt_obj_env(nest);
		} break;
		}

		get = rt_env_lookup(env, id);
		if(get != NULL) {
			if(bind->add)
				rt_obj_add(get->obj, obj, stmt->loc);
			else
				rt_obj_set(&get->obj, obj);

			free(id);
		}
		else
			env_put(env, bind_new(id, obj, stmt->loc));
	} break;

	case syn_v: {
		struct ast_rule_t *syn = stmt->data.syn;
		struct target_list_t *gens, *deps;
		struct val_t *gen, *dep, *iter;
		struct link_t *link;
		struct ast_cmd_t *proc;
		struct rule_t *rule;

		gens = target_list_new();
		deps = target_list_new();

		gen = rt_eval_val(syn->gen, ctx, env, stmt->loc);
		for(iter = gen; iter != NULL; iter = iter->next)
			target_list_add(gens, ctx_target(ctx, iter->spec, iter->str));

		dep = rt_eval_val(syn->dep, ctx, env, stmt->loc);
		for(iter = dep; iter != NULL; iter = iter->next)
			target_list_add(deps, ctx_target(ctx, iter->spec, iter->str));

		rule = ctx_rule(ctx, NULL, gens, deps);
		val_clear(gen);
		val_clear(dep);

		if(syn->cmd != NULL) {
			rule->seq = seq_new();
			ctx->cur = rule;

			for(link = syn->cmd->head; link != NULL; link = link->next) {
				char *out, *in;
				struct ast_pipe_t *iter;
				struct rt_pipe_t *pipe = NULL, **ipipe = &pipe;

				proc = link->val;
				in = proc->in ? rt_eval_str(proc->in, ctx, env, syn->loc) : NULL;
				out = proc->out ? rt_eval_str(proc->out, ctx, env, syn->loc) : NULL;

				for(iter = proc->pipe; iter != NULL; iter = iter->next) {
					*ipipe = rt_pipe_new(rt_eval_val(iter->imm, ctx, env, stmt->loc));
					ipipe = &(*ipipe)->next;
				}

				seq_add(rule->seq, pipe, in, out, proc->append);
			}
		}
	} break;

	case loop_v: {
		struct env_t *nest;
		struct rt_obj_t obj;
		struct loop_t *loop = stmt->data.loop;

		obj = eval_imm(loop->imm, ctx, env, stmt->loc);

		switch(obj.tag) {
		case rt_val_v: {
			struct val_t *iter;

			for(iter = obj.data.val; iter != NULL; iter = iter->next) {
				nest = rt_env_new(env);
				env_put(nest, bind_new(strdup(loop->id), rt_obj_val(val_new(iter->spec, strdup(iter->str))), stmt->loc));
				eval_stmt(loop->body, ctx, nest);
				rt_env_delete(nest);
			}
		} break;

		case rt_env_v: {
			struct env_t *iter;

			for(iter = obj.data.env; iter != NULL; iter = iter->next) {
				nest = rt_env_new(env);
				env_put(nest, bind_new(strdup(loop->id), rt_obj_env(rt_env_dup(iter)), stmt->loc));
				eval_stmt(loop->body, ctx, nest);
				rt_env_delete(nest);
			}
		} break;

		default:
			loc_err(stmt->loc, "Can only iterate over strings and environments.");
		}

		rt_obj_delete(obj);
	} break;

	case print_v: {
		struct val_t *val, *iter;

		val = rt_eval_val(stmt->data.print->imm, ctx, env, stmt->loc);

		for(iter = val; iter != NULL; iter = iter->next)
			print("%s%s", iter->str, iter->next ? " " : "");

		val_clear(val);
	} break;

	case ast_mkdep_v:
		ast_mkdep_eval(stmt->data.mkdep, ctx, env);
		break;

	case block_v: {
		struct env_t *nest;

		nest = rt_env_new(env);
		eval_block(stmt->data.block, ctx, nest);
		rt_env_delete(nest);
	} break;

	case ast_inc_v:
		ast_inc_eval(stmt->data.inc, ctx, env, stmt->loc);
		break;
	}
}

/**
 * Evaluate an immediate value.
 *   @imm: The immediate value.
 *   @ctx: The context.
 *   @env: The environment.
 *   @loc: The location.
 *   &returns: The object.
 */
struct rt_obj_t eval_imm(struct imm_t *imm, struct rt_ctx_t *ctx, struct env_t *env, struct loc_t loc)
{
	struct raw_t *raw;
	struct rt_obj_t obj;

	if(imm->raw == NULL)
		return rt_obj_null();

	obj = eval_raw(imm->raw, ctx, env);
	for(raw = imm->raw->next; raw != NULL; raw = raw->next)
		rt_obj_add(obj, eval_raw(raw, ctx, env), loc);

	return obj;
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

	struct rt_ctx_t *ctx;
	struct env_t *env;
	struct loc_t loc;
};


/*
 * expansion declarations
 */
struct rt_obj_t exp_get(struct exp_t *exp);

char exp_ch(struct exp_t *exp);
char exp_adv(struct exp_t *exp);
char exp_buf(struct exp_t *exp);
char exp_trim(struct exp_t *exp);

void exp_str(struct exp_t *exp);
void exp_escape(struct exp_t *exp);
void exp_quote1(struct exp_t *exp);
void exp_quote2(struct exp_t *exp);
struct rt_obj_t exp_var(struct exp_t *exp);
struct rt_obj_t exp_bind(struct exp_t *exp);
void exp_flat(struct exp_t *exp, struct rt_obj_t obj);

struct loc_t exp_loc(struct exp_t *exp);
__attribute__((noreturn)) void exp_err(struct exp_t *exp, const char *fmt, ...);


/**
 * Get an expaneded a value.
 *   @exp: The expander.
 *   &returns: The value.
 */
struct rt_obj_t exp_get(struct exp_t *exp)
{
	struct rt_obj_t obj;
	struct buf_t tmp;

	tmp = exp->buf;
	exp->buf = buf_new(32);

	if(*exp->str == '$') {
		obj = exp_var(exp);
		if(*exp->str != '\0') {
			exp_flat(exp, obj);
			exp_str(exp);
			obj = rt_obj_val(val_new(false, strdup(buf_done(&exp->buf))));
		}
	}
	else if(*exp->str == '.') {
		exp_buf(exp);
		while(ch_var(exp_ch(exp)))
			exp_buf(exp);

		if(*exp->str != '\0') {
			exp_str(exp);
			obj = rt_obj_val(val_new(false, strdup(buf_done(&exp->buf))));
		}
		else
			obj = rt_obj_val(val_new(true, strdup(buf_done(&exp->buf))));
	}
	else {
		exp_str(exp);
		obj = rt_obj_val(val_new(false, strdup(buf_done(&exp->buf))));
	}

	buf_delete(&exp->buf);
	exp->buf = tmp;

	return obj;
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
	case ',': ch = ','; break;
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
 *   &returns: The object.
 */
struct rt_obj_t exp_var(struct exp_t *exp)
{
	char ch;
	struct rt_obj_t obj;

	ch = exp_adv(exp);
	if(ch == '{') {
		exp_adv(exp);
		obj = exp_bind(exp);

		while((ch = exp_trim(exp)) != '}') {
			const char *id;
			struct buf_t buf;

			if(ch != '.')
				exp_err(exp, "Expected '.' or '}'.");

			buf = buf_new(32);
			buf_ch(&buf, ch);

			exp_adv(exp);
			ch = exp_trim(exp);
			if(!ch_var(ch))
				exp_err(exp, "Expected function/member name.");

			do
				buf_ch(&buf, ch);
			while(ch_var(ch = exp_adv(exp)));

			id = buf_done(&buf);

			switch(obj.tag) {
			case rt_null_v:
				fatal("FIXME exp_var null call");

			case rt_val_v: {
				uint32_t cnt;
				struct bind_t *bind;
				struct loc_t loc;
				struct rt_obj_t *args;

				bind = env_get(exp->env, id);
				if(bind == NULL)
					exp_err(exp, "Unknown function '%s'.", id);
				else if(bind->obj.tag != rt_func_v)
					exp_err(exp, "Variable '%s' is not a function.", id);

				loc = exp_loc(exp);
				args_init(&args, &cnt);
				args_add(&args, &cnt, obj);

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

				switch(bind->obj.tag) {
				case rt_func_v:
					obj = bind->obj.data.func(args, cnt, loc);
					break;

				default:
					unreachable();
				}

				args_delete(args, cnt);
			} break;

			case rt_env_v: {
				struct bind_t *bind;

				bind = env_get(obj.data.env, id + 1);
				if(bind == NULL)
					exp_err(exp, "Unknown member '%s'.", id + 1);

				rt_obj_delete(obj);
				obj = rt_obj_dup(bind->obj);
			} break;

			case rt_func_v:
				fatal("FIXME exp_var func call");
			}

			buf_delete(&buf);
		}

		exp_adv(exp);

		return obj;
	}
	else
		return exp_bind(exp);
}

/**
 * Flatten an object.
 *   @exp: The expander.
 *   @val: The value.
 */
void exp_flat(struct exp_t *exp, struct rt_obj_t obj)
{
	struct val_t *val, *orig;

	if(obj.tag != rt_val_v)
		exp_err(exp, "Cannot convert non-value to a string.");

	val = orig = obj.data.val;

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
 *   &returns: The object.
 */
struct rt_obj_t exp_bind(struct exp_t *exp)
{
	char *id;
	const char *str;
	struct buf_t buf;
	struct bind_t *bind;

	str = exp->str;

	if(*str == '@') {
		struct target_inst_t *inst;
		struct val_t *val = NULL, **iter = &val;

		if(exp->ctx->cur == NULL)
			exp_err(exp, "Variable '$@' can only be used in recipes.");

		for(inst = exp->ctx->cur->gens->inst; inst != NULL; inst = inst->next) {
			*iter = val_new(false, strdup(inst->target->path));
			iter = &(*iter)->next;
		}

		exp_adv(exp);
		return rt_obj_val(val);
	}
	else if(*str == '^') {
		struct target_inst_t *inst;
		struct val_t *val = NULL, **iter = &val;

		if(exp->ctx->cur == NULL)
			exp_err(exp, "Variable '$^' can only be used in recipes.");

		for(inst = exp->ctx->cur->deps->inst; inst != NULL; inst = inst->next) {
			*iter = val_new(false, strdup(inst->target->path));
			iter = &(*iter)->next;
		}

		exp_adv(exp);
		return rt_obj_val(val);
	}
	else if(*str == '<') {
		struct target_inst_t *inst;

		if(exp->ctx->cur == NULL)
			exp_err(exp, "Variable '$<' can only be used in recipes.");

		inst = exp->ctx->cur->deps->inst;
		if(inst == NULL)
			return rt_obj_null();

		exp_adv(exp);
		return rt_obj_val(val_new(inst->target->flags & FLAG_SPEC, strdup(inst->target->path)));
	}
	else if(*str == '*') {
		struct val_t *val, **iter;
		struct rule_inst_t *inst;

		val = NULL;
		iter = &val;
		for(inst = exp->ctx->rules->inst; inst != NULL; inst = inst->next) {
			struct target_inst_t *ref;

			for(ref = inst->rule->gens->inst; ref != NULL; ref = ref->next) {
				if(ref->target->flags & FLAG_SPEC)
					continue;

				*iter = val_new(false, strdup(ref->target->path));
				iter = &(*iter)->next;
			}
		}

		*iter = NULL;
		exp_adv(exp);
		return rt_obj_val(val);
	}

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

	return rt_obj_dup(bind->obj);
}


/**
 * Retrieve a location for thes string expansion.
 *   @exp: The string expansion.
 *   &returns: The location.
 */
struct loc_t exp_loc(struct exp_t *exp)
{
	return loc_off(exp->loc, exp->str - exp->orig);
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
 *   &returns: The object.
 */
struct rt_obj_t eval_raw(struct raw_t *raw, struct rt_ctx_t *ctx, struct env_t *env)
{
	struct exp_t exp;

	exp.orig = exp.str = raw->str;
	exp.loc = raw->loc;
	exp.ctx = ctx;
	exp.env = env;

	return exp_get(&exp);
}


/**
 * Evaluate an immediate to a value.
 *   @imm: The immediate.
 *   @ctx: The context.
 *   @env: The environment.
 *   @loc: The location for error information.
 *   &returns: The value.
 */
struct val_t *rt_eval_val(struct imm_t *imm, struct rt_ctx_t *ctx, struct env_t *env, struct loc_t loc)
{
	struct rt_obj_t obj;

	obj = eval_imm(imm, ctx, env, loc);
	if(obj.tag == rt_null_v)
		return NULL;
	else if(obj.tag != rt_val_v)
		loc_err(loc, "String values required.");

	return obj.data.val;
}

/**
 * Evaluate an raw to a string.
 *   @raw: The raw.
 *   @ctx: The context.
 *   @env: The environment.
 *   @loc: The location for error information.
 *   &returns: The string.
 */
char *rt_eval_str(struct raw_t *raw, struct rt_ctx_t *ctx, struct env_t *env, struct loc_t loc)
{
	char *str;
	struct val_t *val;
	struct rt_obj_t obj;

	obj = eval_raw(raw, ctx, env);
	if(obj.tag != rt_val_v)
		loc_err(loc, "String required.");

	val = obj.data.val;
	if((val == NULL) || (val->next != NULL))
		loc_err(loc, "String required.");

	str = val->str;
	free(val);

	return str;
}


/**
 * Initialize arguments.
 *   @args: The arguments reference.
 *   @cnt: The number of arguments reference.
 */
void args_init(struct rt_obj_t **args, uint32_t *cnt)
{
	*args = malloc(0);
	*cnt = 0;
}

/**
 * Add to arguments.
 *   @args: The arguments reference.
 *   @cnt: The number of arguments reference.
 *   @obj: Consumed. The object.
 */
void args_add(struct rt_obj_t **args, uint32_t *cnt, struct rt_obj_t obj)
{
	*args = realloc(*args, (*cnt + 1) * sizeof(struct rt_obj_t));
	(*args)[(*cnt)++] = obj;
}

/**
 * Delete arguments.
 *   @args: The arguments.
 *   @cnt: The number of arguments.
 */
void args_delete(struct rt_obj_t *args, uint32_t cnt)
{
	uint32_t i;

	for(i = 0; i < cnt; i++)
		rt_obj_delete(args[i]);

	free(args);
}



/**
 * Perform simple text substitution.
 *   @args: The arguments.
 *   @cnt: The number of arguments.
 *   @loc: The call location for error reporting.
 */
struct rt_obj_t fn_sub(struct rt_obj_t *args, uint32_t cnt, struct loc_t loc)
{
	struct buf_t buf;
	struct val_t *val, *ret, **iter;
	const char *get, *put, *str, *find;

	if(cnt != 3)
		loc_err(loc, "Function `.sub` requires 2 arguments.");
	else if((args[0].tag != rt_val_v) || (args[1].tag != rt_val_v) || (args[2].tag != rt_val_v))
		loc_err(loc, "Function `.sub` requires string values as arguments.");
	else if((val_len(args[1].data.val) != 1) || (val_len(args[2].data.val) != 1))
		loc_err(loc, "Function `.sub` requires string values as arguments.");

	iter = &ret;
	get = args[1].data.val->str;
	put = args[2].data.val->str;

	for(val = args[0].data.val; val != NULL; val = val->next) {
		str = val->str;
		buf = buf_new(strlen(val->str) + 1);

		find = strstr(str, get);
		while(find != NULL) {
			buf_mem(&buf, str, find - str);
			buf_str(&buf, put);

			str = find + strlen(get);
			find = strstr(str, get);
		}
		buf_str(&buf, str);

		*iter = val_new(val->spec, buf_done(&buf));
		iter = &(*iter)->next;
	}

	*iter = NULL;

	return rt_obj_val(ret);
}

/**
 * Compute the pre and post lengths of a pattern.
 *   @str: The string.
 *   @pre: Out. The pre-pattern length.
 *   @post: Out The post-pattern length.
 */
bool pat_len(const char *str, uint32_t *pre, uint32_t *post)
{
	const char *find;

	find = strchr(str, '%');
	if((find == NULL) || (strrchr(str, '%') != find))
		return false;

	*pre = find - str;
	*post = strlen(str) - *pre - 1;
	return true;
}


struct rt_obj_t fn_pat(struct rt_obj_t *args, uint32_t cnt, struct loc_t loc)
{
	uint32_t len, min, spre, spost, slen, rpre, rpost, rlen;
	struct buf_t buf;
	struct val_t *val, *ret, **iter;
	char *pat, *repl;

	if(cnt != 3)
		loc_err(loc, "Function `.pat` requires 2 arguments.");
	else if((args[0].tag != rt_val_v) || (args[1].tag != rt_val_v) || (args[2].tag != rt_val_v))
		loc_err(loc, "Function `.sub` requires string values as arguments.");
	else if((val_len(args[1].data.val) != 1) || (val_len(args[2].data.val) != 1))
		loc_err(loc, "Function `.sub` requires string values as arguments.");

	pat = args[1].data.val->str;
	repl = args[2].data.val->str;
	slen = strlen(pat);
	rlen = strlen(repl);

	if(!pat_len(pat, &spre, &spost) || !pat_len(repl, &rpre, &rpost))
		loc_err(loc, "Function `.pat` requires patterns as arguments (must contain a single '%').");

	iter = &ret;
	min = spre + spost;

	for(val = args[0].data.val; val != NULL; val = val->next) {
		len = strlen(val->str);
		if((len > min) && (memcmp(val->str, pat, spre) == 0) && (strcmp(val->str + len - spost, pat + slen - spost) == 0)) {
			buf = buf_new(32);
			buf_mem(&buf, repl, rpre);
			buf_mem(&buf, val->str + spre, len - spost - spre);
			buf_str(&buf, repl + rlen - rpost);

			*iter = val_new(val->spec, buf_done(&buf));
		}
		else
			*iter = val_new(val->spec, strdup(val->str));

		iter = &(*iter)->next;
	}

	*iter = NULL;

	return rt_obj_val(ret);
}
