#include "inc.h"


/**
 * Parse from the top.
 *   @rd: The reader.
 *   @ctx: The context.
 *   @ns: The namespace.
 */
void par_top(struct rd_t *rd, struct ctx_t *ctx, struct ns_t *ns)
{
	for(;;) {
		struct tok_t *tok = rd_top(rd);

		if(tok->id == TOK_EOF)
			break;

		if(tok->id == TOK_ID) {
			if(strcmp(tok->str, "print") == 0)
				par_print(rd, ns);
			else if(rd_get(rd, 1)->id == '=')
				par_assign(rd, ns);
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
			loc_err(tok->loc, "Unexpected input '%s'.", tok->str);
	}
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

		target_list_add(gens, ctx_target(ctx, spec, tok->str));
		tok = rd_adv(rd, 1);
	} while((tok->id == TOK_ID) || (tok->id == TOK_STR));

	if(tok->id != ':')
		loc_err(tok->loc, "Expected string or ':'.");

	tok = rd_adv(rd, 1);
	while((tok->id == TOK_ID) || (tok->id == TOK_STR)) {
		spec = ((tok->id == TOK_ID) && (tok->str[0] == '.'));
		target_list_add(deps, ctx_target(ctx, spec, tok->str));
		tok = rd_adv(rd, 1);
	}

	if(tok->id == '{') {
		rd_adv(rd, 1);
		seq = par_seq(rd, ctx);
	}
	else if(tok->id == ';') {
		seq = seq_new();
		rd_adv(rd, 1);
	}
	else
		loc_err(tok->loc, "Expected string, ';', or '{'.");

	ctx_rule(ctx, NULL, gens, deps, seq);
}

struct val_t *par_val(struct rd_t *rd, struct ctx_t *ctx)
{
	struct tok_t *tok;
	struct val_t *val = NULL, **ival = &val;

	tok = rd_top(rd);
	if((tok->id != TOK_ID) && (tok->id != TOK_STR))
		return NULL;

	do {
		*ival = val_new(strdup(tok->str));
		ival = &(*ival)->next;
		tok = rd_adv(rd, 1);
	} while((tok->id == TOK_ID) || (tok->id == TOK_STR));

	if(tok->id != ';')
		loc_err(tok->loc, "Expected string or ';'.");

	rd_adv(rd, 1);

	return val;
}

/**
 * Parse a command sequence.
 *   @rd: The reader.
 *   @ctx: The context.
 *   &returns: The sequence.
 */
struct seq_t *par_seq(struct rd_t *rd, struct ctx_t *ctx)
{
	struct seq_t *seq;
	struct tok_t *tok;
	struct val_t *val;

	seq = seq_new();

	while((tok = rd_top(rd))->id != '}') {
		val = par_val(rd, ctx);
		if(val == NULL)
			loc_err(tok->loc, "Expected command or '}'.");

		seq_add(seq, val);
	}

	rd_adv(rd, 1);

	return seq;
}

/**
 * Parse a print statement.
 *   @rd: The reader.
 *   @ns: The namespace.
 */
void par_print(struct rd_t *rd, struct ns_t *ns)
{
	struct tok_t *tok;
	struct val_t *iter, *val = NULL, **ival = &val;

	tok = rd_adv(rd, 1);
	while((tok->id == TOK_ID) || (tok->id == TOK_STR)) {
		*ival = val_new(strdup(tok->str));
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

void par_assign(struct rd_t *rd, struct ns_t *ns)
{
	char *id;
	struct tok_t *tok;
	struct bind_t **bind;

	assert(rd_get(rd, 0)->id == TOK_ID);
	assert(rd_get(rd, 1)->id == '=');

	tok = rd_top(rd);
	id = strdup(tok->str);
	rd_adv(rd, 2);

	bind = ns_lookup(ns, id);
	if(*bind != NULL)
		loc_err(tok->loc, "Duplicate declaration of '%s'.", id);

	tok = rd_top(rd);
	if((tok->id == TOK_ID) || (tok->id == TOK_STR)) {
		struct val_t *val = NULL, **ival = &val;

		do {
			*ival = val_new(strdup(tok->str));
			ival = &(*ival)->next;
			tok = rd_adv(rd, 1);
		} while((tok->id == TOK_ID) || (tok->id == TOK_STR));

		if(tok->id != ';')
			loc_err(tok->loc, "Expected string or ';'.");

		rd_adv(rd, 1);
		*bind = bind_val(id, val);
	}
	else if(tok->id == '[') {
		cli_err("FIXME Stub");
	}
	else
		loc_err(tok->loc, "Expected string or array.");
}
