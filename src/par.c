#include "inc.h"

/**
 * Parse from the top.
 *   @rd: The reader.
 *   @ctx: The context.
 *   @ns: The namespace.
 */
void par_top(struct rd_t *rd, struct ctx_t *ctx, struct ns_t *ns)
{
	struct tok_t *tok;

	for(;;) {
		tok = rd_top(rd);
		if(tok->id == TOK_EOF)
			break;

		par_stmt(rd, ctx, ns);
	}
}

/**
 * Parse a block of statements.
 *   @rd: The reader.
 *   @ctx: The context.
 *   @ns: The namespace.
 */
void par_block(struct rd_t *rd, struct ctx_t *ctx, struct ns_t *ns)
{
	struct tok_t *tok;

	assert(rd_get(rd, 0)->id == '{');

	rd_adv(rd, 1);
	for(;;) {
		tok = rd_top(rd);
		if(tok->id == '}')
			break;

		par_stmt(rd, ctx, ns);
	}

	rd_adv(rd, 1);
}

/**
 * Parse a single statement.
 *   @rd: The reader.
 *   @ctx: The context.
 *   @ns: The namespace.
 */
void par_stmt(struct rd_t *rd, struct ctx_t *ctx, struct ns_t *ns)
{
	struct tok_t *tok = rd_top(rd);

	if(tok->id == TOK_ID) {
		if(strcmp(tok->str, "print") == 0)
			par_print(rd, ctx, ns);
		else if(strcmp(tok->str, "dir") == 0)
			par_dir(rd, ctx, ns);
		else if(rd_get(rd, 1)->id == '=')
			par_assign(rd, ctx, ns);
		else if(rd_get(rd, 1)->id == ':')
			par_rule(rd, ctx, ns);
		else
			par_rule(rd, ctx, ns);
	}
	else if(tok->id == TOK_STR)
		par_rule(rd, ctx, ns);
	else if(tok->id == ';')
		rd_adv(rd, 1);
	else
		loc_err(tok->loc, "Expected statement.");
}

/**
 * Parse a rule.
 *   @rd: The reader.
 *   @ctx: The context.
 *   @ns: The namespace.
 */
void par_rule(struct rd_t *rd, struct ctx_t *ctx, struct ns_t *ns)
{
	bool spec = false;
	struct target_list_t *gens, *deps;
	struct seq_t *seq;
	struct tok_t *tok;

	gens = target_list_new();
	deps = target_list_new();

	tok = rd_top(rd);
	if((tok->id == TOK_ID) && (tok->str[0] == '.'))
		spec = true;

	do {
		if(((tok->id == TOK_ID) && (tok->str[0] == '.')) != spec)
			loc_err(tok->loc, "Cannot mix special and normal targets.");

		target_list_add(gens, ctx_target(ctx, spec, ctx_str(ctx, ns, tok)));
		tok = rd_adv(rd, 1);
	} while((tok->id == TOK_ID) || (tok->id == TOK_STR));

	if(tok->id != ':')
		loc_err(tok->loc, "Expected string or ':'.");

	tok = rd_adv(rd, 1);
	while((tok->id == TOK_ID) || (tok->id == TOK_STR)) {
		spec = ((tok->id == TOK_ID) && (tok->str[0] == '.'));
		target_list_add(deps, ctx_target(ctx, spec, ctx_str(ctx, ns, tok)));
		tok = rd_adv(rd, 1);
	}

	if(tok->id == '{') {
		rd_adv(rd, 1);
		ctx->gens = gens;
		ctx->deps = deps;
		seq = par_seq(rd, ctx, ns);
		ctx->gens = ctx->deps = NULL;
	}
	else if(tok->id == ';') {
		seq = seq_new();
		rd_adv(rd, 1);
	}
	else
		loc_err(tok->loc, "Expected string, ';', or '{'.");

	ctx_rule(ctx, NULL, gens, deps, seq);
}

/**
 * Parse a command sequence.
 *   @rd: The reader.
 *   @ctx: The context.
 *   @ns: The namespace.
 *   &returns: The sequence.
 */
struct seq_t *par_seq(struct rd_t *rd, struct ctx_t *ctx, struct ns_t *ns)
{
	struct seq_t *seq;
	struct tok_t *tok;
	struct val_t *val;

	seq = seq_new();

	while((tok = rd_top(rd))->id != '}') {
		val = par_val(rd, ctx, ns);
		if(val == NULL)
			loc_err(tok->loc, "Expected command or '}'.");

		seq_add(seq, val);
	}

	rd_adv(rd, 1);

	return seq;
}

/**
 * Parse a value.
 *   @rd: The reader.
 *   @ctx: The context.
 *   @ns: The namespace.
 *   &returns: The value.
 */
struct val_t *par_val(struct rd_t *rd, struct ctx_t *ctx, struct ns_t *ns)
{
	struct tok_t *tok;
	struct val_t *val = NULL, **ival = &val;

	tok = rd_top(rd);
	if((tok->id != TOK_ID) && (tok->id != TOK_STR))
		return NULL;

	do {
		*ival = val_new(false, strdup(ctx_str(ctx, ns, tok)));
		ival = &(*ival)->next;
		tok = rd_adv(rd, 1);
	} while((tok->id == TOK_ID) || (tok->id == TOK_STR));

	if(tok->id != ';')
		loc_err(tok->loc, "Expected string or ';'.");

	rd_adv(rd, 1);

	return val;
}


/**
 * Parse a print statement.
 *   @rd: The reader.
 *   @ctx: The context.
 *   @ns: The namespace.
 */
void par_print(struct rd_t *rd, struct ctx_t *ctx, struct ns_t *ns)
{
	struct tok_t *tok;
	struct val_t *iter, *val = NULL, **ival = &val;

	tok = rd_adv(rd, 1);
	while((tok->id == TOK_ID) || (tok->id == TOK_STR)) {
		*ival = val_new(false, strdup(ctx_str(ctx, ns, tok)));
		ival = &(*ival)->next;
		tok = rd_adv(rd, 1);
	}

	if(tok->id != ';')
		loc_err(tok->loc, "Expected string or ';'.");

	for(iter = val; iter != NULL; iter = iter->next) {
		printf("%s", iter->str);

		if(iter->next != NULL)
			printf(" ");
	}
	printf("\n");

	val_clear(val);
	rd_adv(rd, 1);
}

/**
 * Parse a directory.
 *   @rd: The reader.
 *   @ctx: The context.
 *   @ns: The namespace.
 */
void par_dir(struct rd_t *rd, struct ctx_t *ctx, struct ns_t *ns)
{
	bool match;
	const char *str;
	struct tok_t *tok;

	tok = rd_adv(rd, 1);
	if((tok->id != TOK_ID) && (tok->id != TOK_STR) && (tok->id != TOK_STR2))
		loc_err(tok->loc, "Missing directory name.");

	str = ctx_str(ctx, ns, tok);
	match = ((ctx->opt->dir != NULL)) && (strcmp(ctx->opt->dir, str) == 0);

	tok = rd_adv(rd, 1);
	if((tok->id == TOK_ID) && (strcmp(tok->str, "default") == 0)) {
		rd_adv(rd, 1);
		match = (ctx->opt->dir == NULL);
	}

	if(match)
		ctx->dir = strdup(str);

	tok = rd_top(rd);
	if(tok->id == '{')
		par_block(rd, ctx, match ? ns : NULL);
	else if(tok->id != ';')
		loc_err(tok->loc, "Expected ';' or '{'.");
}

/**
 * Parse an statement.
 *   @rd: The reader.
 *   @ctx: The context.
 *   @ns: The namespace.
 */
void par_assign(struct rd_t *rd, struct ctx_t *ctx, struct ns_t *ns)
{
	char *id;
	struct tok_t *tok;
	struct bind_t **bind;

	assert(rd_get(rd, 0)->id == TOK_ID);
	assert(rd_get(rd, 1)->id == '=');

	tok = rd_top(rd);
	id = strdup(tok->str);
	rd_adv(rd, 2);

	if(ns != NULL) {
		bind = ns_lookup(ns, id);
		if(*bind != NULL)
			loc_err(tok->loc, "Duplicate declaration of '%s'.", id);
	}

	tok = rd_top(rd);
	if((tok->id == TOK_ID) || (tok->id == TOK_STR)) {
		struct val_t *val = NULL, **ival = &val;

		do {
			*ival = val_new(false, strdup(ctx_str(ctx, ns, tok)));
			ival = &(*ival)->next;
			tok = rd_adv(rd, 1);
		} while((tok->id == TOK_ID) || (tok->id == TOK_STR));

		if(tok->id != ';')
			loc_err(tok->loc, "Expected string or ';'.");

		rd_adv(rd, 1);

		if(ns != NULL)
			*bind = bind_val(id, val);
	}
	else if(tok->id == '[') {
		cli_err("FIXME Stub");
	}
	else
		loc_err(tok->loc, "Expected string or array.");
}
