#include "inc.h"


/**
 * Create a loop.
 *   @id: Consumed. The identifier.
 *   @imm: Consumed. The immediate value.
 *   @body: Consumed. The body statement.
 *   @loc: The location.
 *   &returns: The loop.
 */
struct loop_t *loop_new(char *id, struct imm_t *imm, struct ast_stmt_t *body, struct loc_t loc)
{
	struct loop_t *loop;

	loop = malloc(sizeof(struct loop_t));
	*loop = (struct loop_t){ id, imm, body, loc };

	return loop;
}

/**
 * Delete a loop.
 *   @loop: The loop.
 */
void loop_delete(struct loop_t *loop)
{
	imm_delete(loop->imm);
	stmt_delete(loop->body);
	free(loop->id);
	free(loop);
}


/**
 * Create a map.
 *   @cmp: The comparison function.
 *   @del: The deletion function.
 */
struct map0_t *map0_new(cmp_f cmp, del_f del)
{
	struct map0_t *map;

	map = malloc(sizeof(struct map0_t));
	map->cmp = cmp;
	map->del = del;
	map->entry = NULL;

	return map;
}

/**
 * Delete a map.
 *   @map: The map.
 */
void map0_delete(struct map0_t *map)
{
	struct entry0_t *entry;

	while(map->entry != NULL) {
		map->entry = (entry = map->entry)->next;
		map->del(entry->val);
		free(entry);
	}

	free(map);
}


/**
 * Add a key-value pair to the map.
 *   @map: The map.
 *   @key: The key.
 *   @val: The value.
 */
void map0_add(struct map0_t *map, const void *key, void *val)
{
	struct entry0_t *entry;

	entry = malloc(sizeof(struct entry0_t));
	entry->key = key;
	entry->val = val;
	entry->next = map->entry;
	map->entry = entry;
}

/**
 * Try to retrieve a value from the map.
 *   @map: The map.
 *   @key: The key.
 *   &returns: The value if found, null otherwise.
 */
void *map0_get(struct map0_t *map, const void *key)
{
	struct entry0_t *entry;

	for(entry = map->entry; entry != NULL; entry = entry->next) {
		if(map->cmp(entry->key, key) == 0)
			return entry->val;
	}

	return NULL;
}

/**
 * Try to remove a value from the map.
 *   @map: The map.
 *   @key: The key.
 *   &returns: The value if found, null otherwise.
 */
void *map_rem(struct map0_t *map, const void *key)
{
	void *val;
	struct entry0_t **entry, *tmp;

	for(entry = &map->entry; *entry != NULL; entry = &(*entry)->next) {
		if(map->cmp((*entry)->key, key) != 0)
			continue;

		*entry = (tmp = *entry)->next;
		val = tmp->val;
		free(tmp);

		return val;
	}

	return NULL;
}


/**
 * Create a print statement.
 *   @imm: Consumed. The immediate value.
 *   &returns: The print statement.
 */
struct print_t *print_new(struct imm_t *imm)
{
	struct print_t *print;

	print = malloc(sizeof(struct print_t));
	print->imm = imm;

	return print;
}

/**
 * Delete a print statement.
 *   @print: The print.
 */
void print_delete(struct print_t *print)
{
	imm_delete(print->imm);
	free(print);
}


/**
 * Creat an immediate value.
 *   &returns: The immediate.
 */
struct imm_t *imm_new(void)
{
	struct imm_t *list;

	list = malloc(sizeof(struct imm_t));
	list->raw = NULL;

	return list;
}

/**
 * Delete an immediate value.
 *   @imm: The immediate.
 */
void imm_delete(struct imm_t *imm)
{
	raw_clear(imm->raw);
	free(imm);
}


/**
 * Retrieve the length of an immediate value.
 *   @imm: The immate.
 *   &returns: The length.
 */
uint32_t imm_len(struct imm_t *imm)
{
	uint32_t n;
	struct raw_t *raw;

	n = 0;
	for(raw = imm->raw; raw != NULL; raw = raw->next)
		n++;

	return n;
}


/**
 * Create an raw string.
 *   @spec: Special value flag.
 *   @var: Variable flag.
 *   @str: Consumed. The string.
 *   @loc: The location.
 *   &returns: The raw string.
 */
struct raw_t *raw_new(bool spec, bool var, char *str, struct loc_t loc)
{
	struct raw_t *raw;

	raw = malloc(sizeof(struct raw_t));
	raw->spec = spec;
	raw->var = var;
	raw->str = str;
	raw->loc = loc;
	raw->next = NULL;

	return raw;
}

/**
 * Duplicate a raw string.
 *   @raw: The raw string.
 *   &returns: The duplicated raw string.
 */
struct raw_t *raw_dup(const struct raw_t *raw)
{
	return raw_new(raw->spec, raw->var, strdup(raw->str), raw->loc);
}

/**
 * Delete an raw string.
 *   @raw: The raw string.
 */
void raw_delete(struct raw_t *raw)
{
	free(raw->str);
	free(raw);
}

/**
 * Clear a list of raw strings.
 *   @raw: The raw string list.
 */
void raw_clear(struct raw_t *raw)
{
	struct raw_t *tmp;

	while(raw != NULL) {
		raw = (tmp = raw)->next;
		raw_delete(tmp);
	}
}


/**
 * Reader structure.
 *   @file: The file.
 *   @ch: The character.
 *   @loc: The location.
 *   @tok: The current token.
 *   @str: The token string.
 *   @len, max: The string length and max.
 */
struct rd_t {
	FILE *file;
	int ch;

	struct loc_t loc, tloc;

	int tok;

	char *str;
	uint32_t len, max;
};

/*
 * reader declaraions
 */
int rd_ch(struct rd_t *rd);
int rd_tok(struct rd_t *rd);

void rd_reset(struct rd_t *rd);
int rd_push(struct rd_t *rd, char ch);
int rd_buf(struct rd_t *rd);

struct ast_block_t *rd_top(struct rd_t *rd);
struct ast_stmt_t *rd_stmt(struct rd_t *rd);
struct imm_t *rd_imm(struct rd_t *rd);
struct raw_t *rd_raw(struct rd_t *rd);


/**
 * Load a hammer file from the top.
 *   @path: The path.
 *   &returns: The block.
 */
struct ast_block_t *ham_load(const char *path)
{
	struct rd_t rd;
	struct ast_block_t *block;

	rd.file = fopen(path, "r");
	if(rd.file == NULL)
		return NULL;

	rd.loc.path = path;
	rd.loc.lin = 1;
	rd.loc.col = 0;
	rd.str = malloc(64);
	rd.len = 0;
	rd.max = 64;

	rd_ch(&rd);
	rd.tok = rd_tok(&rd);

	block = rd_top(&rd);

	fclose(rd.file);

	free(rd.str);

	return block;
}


/**
 * Read a character.
 *   @rd: The reader.
 *   &returns: The next character.
 */
int rd_ch(struct rd_t *rd)
{
	rd->ch = fgetc(rd->file);
	if(rd->ch == '\n') {
		rd->loc.lin++;
		rd->loc.col = 0;
	}
	else
		rd->loc.col++;

	return rd->ch;
}


/**
 * Reset the reader's string buffer.
 *   @rd: The reader.
 */
void rd_reset(struct rd_t *rd)
{
	rd->len = 0;
}

/**
 * Push a character onto the reader's string buffer.
 *   @rd: The reader.
 *   @ch: The character.
 *   &returns: The next character.
 */
int rd_push(struct rd_t *rd, char ch)
{
	if(rd->len >= rd->max)
		rd->str = realloc(rd->str, rd->max *= 2);

	rd->str[rd->len++] = ch;

	return rd->ch;
}

/**
 * Buffer the next character from the reader and advance.
 *   @rd: The reader.
 *   &returns: The next character.
 */
int rd_buf(struct rd_t *rd)
{
	rd_push(rd, rd->ch);

	return rd_ch(rd);
}

struct sym_t {
	int tok;
	const char *str;
};

/**
 * Compound token IDs.
 */
#define TOK_STR     0x1000
#define TOK_VAR     0x1001
#define TOK_SPEC    0x1002
#define TOK_DIR     0x2000
#define TOK_FOR     0x2001
#define TOK_IF      0x2002
#define TOK_ELIF    0x2003
#define TOK_ELSE    0x2004
#define TOK_PRINT   0x2005
#define TOK_DEF     0x2006
#define TOK_MKDEP   0x2007
#define TOK_INCLUDE 0x2008
#define TOK_IMPORT  0x2009
#define TOK_SHR     0x3000
#define TOK_SHL     0x3001
#define TOK_ADDEQ   0x4000
#define TOK_EOF     0x7FFF

struct sym_t syms[] = {
	{ TOK_SHR,   ">>" },
	{ TOK_SHL,   "<<" },
	{ TOK_ADDEQ, "+=" },
	{ '{',       "{" },
	{ '}',       "}" },
	{ ':',       ":" },
	{ ';',       ";" },
	{ '=',       "=" },
	{ '<',       "<" },
	{ '>',       ">" },
	{ '|',       "|" },
	{ '?',       "?" },
	{ 0,         NULL }
};

struct sym_t keys[] = {
	{ TOK_DIR,     "dir"      },
	{ TOK_FOR,     "for"      },
	{ TOK_IF,      "if"       },
	{ TOK_ELIF,    "elif"     },
	{ TOK_ELSE,    "else"     },
	{ TOK_PRINT,   "print"    },
	{ TOK_DEF  ,   "default"  },
	{ TOK_MKDEP,   "makedep"  },
	{ TOK_INCLUDE, "include"  },
	{ TOK_IMPORT,  "import"   },
	{ 0,            NULL      }
};


bool rd_var(struct rd_t *rd);
void rd_str(struct rd_t *rd);
bool rd_quote1(struct rd_t *rd);
bool rd_quote2(struct rd_t *rd);
bool rd_escape(struct rd_t *rd);


/**
 * Read an escaped character.
 *   @rd: The reader.
 *   &returns: True if an escape character, false otherwise.
 */
bool rd_escape(struct rd_t *rd)
{
	if(rd->ch != '\\')
		return false;

	if(strchr("tn'\" ,$", rd_ch(rd)) == NULL)
		loc_err(rd->loc, "Invalid escape character '\\%c'.", rd->ch);

	rd_push(rd, '\\');
	rd_buf(rd);

	return true;
}

/**
 * Read a string.
 *   @rd: The reader.
 */
void rd_str(struct rd_t *rd)
{
	for(;;) {
		if(rd->ch == '$')
			rd_var(rd);
		else if(rd->ch == '\'')
			rd_quote1(rd);
		else if(rd->ch == '"')
			rd_quote2(rd);
		else if(rd->ch == '\\')
			rd_escape(rd);
		else if(ch_str(rd->ch))
			rd_buf(rd);
		else
			break;
	}
}

/**
 * Read a variable string.
 *   @rd: The reader.
 *   &returns: True if a variable string.
 */
bool rd_var(struct rd_t *rd)
{
	if(rd->ch != '$')
		return false;

	rd_buf(rd);
	if(rd->ch == '{') {
		rd_buf(rd);

		for(;;) {
			if(rd->ch == '\\')
				rd_escape(rd);
			else if(rd->ch == '}')
				break;
			else
				rd_buf(rd);
		}

		rd_buf(rd);
	}
	else if(ch_var(rd->ch)) {
		do
			rd_buf(rd);
		while(ch_var(rd->ch));
	}
	else if(strchr("@^<*", rd->ch) != NULL)
		rd_buf(rd);
	else
		loc_err(rd->loc, "Invalid variable name.");

	return true;
}

/**
 * Read a single-quoted string, including the quote.
 *   @rd: The reader.
 *   &returns: True if a double-quoted string.
 */
bool rd_quote1(struct rd_t *rd)
{
	if(rd->ch != '\'')
		return false;

	rd_buf(rd);

	for(;;) {
		if(rd->ch == '\\')
			rd_escape(rd);
		else if(rd->ch == '\'')
			break;
		else if((rd->ch == '\n') || (rd->ch < 0))
			loc_err(rd->loc, "Unterminated quote.", rd->ch);
		else
			rd_buf(rd);
	}

	rd_buf(rd);

	return true;
}

/**
 * Read a double-quoted string, including the quote.
 *   @rd: The reader.
 *   &returns: True if a double-quoted string.
 */
bool rd_quote2(struct rd_t *rd)
{
	if(rd->ch != '"')
		return false;

	rd_buf(rd);

	for(;;) {
		if(rd->ch == '\\')
			rd_escape(rd);
		else if(rd->ch == '$')
			rd_var(rd);
		else if(rd->ch == '"')
			break;
		else if((rd->ch == '\n') || (rd->ch < 0))
			loc_err(rd->loc, "Unterminated quote.", rd->ch);
		else
			rd_buf(rd);
	}

	rd_buf(rd);

	return true;
}

/**
 * Read the next token.
 *   @rd: The reader.
 *   &returns: The token.
 */
int rd_tok(struct rd_t *rd)
{
	char ch, peek;
	struct sym_t *sym;

	for(;;) {
		while(ch_space(rd->ch))
			rd_ch(rd);

		if(rd->ch != '#')
			break;

		do
			rd_ch(rd);
		while((rd->ch >= 0) && (rd->ch != '\n'));
	}

	rd_reset(rd);
	rd->tloc = rd->loc;

	ch = rd->ch;
	peek = '\0';
	for(sym = syms; sym->tok != 0; sym++) {
		if(ch != sym->str[0])
			continue;

		if(peek == '\0')
			peek = rd_ch(rd);

		if(sym->str[1] == '\0')
			return rd->tok = sym->tok;
		else if(sym->str[1] != peek)
			continue;

		rd_ch(rd);
		return rd->tok = sym->tok;
	}

	if(ch_str(rd->ch) || (rd->ch == '$') || (rd->ch == '"') || (rd->ch == '\'')) {
		rd_str(rd);
		rd_push(rd, '\0');

		for(sym = keys; sym->tok != 0; sym++) {
			if(strcmp(rd->str, sym->str) == 0)
				return rd->tok = sym->tok;
		}

		return rd->tok = TOK_STR;
	}
	else if(rd->ch < 0)
		return rd->tok = TOK_EOF;
	else
		fatal("FIXME boo %c", rd->ch);

	return rd->tok;
}


/**
 * Read a block.
 *   @rd: The reader.
 *   &returns: The block.
 */
struct ast_block_t *rd_block(struct rd_t *rd)
{
	struct ast_block_t *block;
	struct ast_stmt_t **stmt;

	if(rd->tok != '{')
		loc_err(rd->tloc, "Expected ';' or '{'.");

	rd_tok(rd);
	block = ast_block_new();
	stmt = &block->stmt;

	while(rd->tok != '}') {
		*stmt = rd_stmt(rd);
		stmt = &(*stmt)->next;
	}

	rd_tok(rd);

	return block;
}


/**
 * Read from the top.
 *   @rd: The reader.
 *   &returns: The block.
 */
struct ast_block_t *rd_top(struct rd_t *rd)
{
	struct ast_block_t *block;
	struct ast_stmt_t **stmt;

	block = ast_block_new();
	stmt = &block->stmt;

	while(rd->tok != TOK_EOF) {
		*stmt = rd_stmt(rd);
		stmt = &(*stmt)->next;
	}

	return block;
}

/**
 * Read a statement.
 *   @rd: The reader.
 *   &returns: The statement.
 */
struct ast_stmt_t *rd_stmt(struct rd_t *rd)
{
	struct loc_t loc;
	enum stmt_e tag;
	union stmt_u data;

	loc = rd->loc;
	if((rd->tok == TOK_STR) || (rd->tok == TOK_SPEC) || (rd->tok == TOK_VAR)) {
		struct imm_t *lhs, *rhs;

		lhs = rd_imm(rd);

		if((rd->tok == '=') || (rd->tok == TOK_ADDEQ)) {
			struct raw_t *id;
			struct ast_bind_t *bind;
			bool add = (rd->tok == TOK_ADDEQ);

			if(imm_len(lhs) == 0)
				loc_err(rd->tloc, "Missing variable name.");
			else if(imm_len(lhs) >= 2)
				loc_err(rd->tloc, "Invalid variable name.");

			id = raw_dup(lhs->raw);
			imm_delete(lhs);
			rd_tok(rd);

			if((rd->tok == TOK_STR) && (strcmp(rd->str, "env") == 0)) {
				struct ast_block_t *block;

				rd_tok(rd);
				block = rd_block(rd);

				bind = ast_bind_block(id, block, add);
			}
			else {
				rhs = rd_imm(rd);
				bind = ast_bind_val(id, rhs, add);

				if(rd->tok != ';')
					loc_err(rd->tloc, "Expected ';'.");

				rd_tok(rd);
			}

			tag = ast_bind_v;
			data.bind = bind;
		}
		else if(rd->tok == ':') {
			loc = rd->tloc;
			rd_tok(rd);
			rhs = rd_imm(rd);
	
			tag = syn_v;
			data.syn = ast_rule_new(lhs, rhs, loc);
			if(rd->tok == '{') {
				data.syn->cmd = list_new((del_f)ast_cmd_delete);

				rd_tok(rd);
				while(rd->tok != '}') {
					struct ast_cmd_t *proc;
					struct ast_pipe_t *pipe, **ipipe;

					pipe = NULL;
					ipipe = &pipe;

					*ipipe = ast_pipe_new(rd_imm(rd));
					ipipe = &(*ipipe)->next;

					while(rd->tok == '|') {
						rd_tok(rd);

						*ipipe = ast_pipe_new(rd_imm(rd));
						ipipe = &(*ipipe)->next;
					}

					proc = ast_cmd_new(pipe);
					list_add(data.syn->cmd, proc);

					while(rd->tok != ';') {
						if((rd->tok == '>') || (rd->tok == TOK_SHR)) {
							if(proc->out != NULL)
								loc_err(rd->tloc, "Output direct already given.");

							if(rd->tok == TOK_SHR)
								proc->append = true;

							rd_tok(rd);
							proc->out = rd_raw(rd);
							if(proc->out == NULL)
								loc_err(rd->tloc, "Missing output file path.");

						}
						else
							loc_err(rd->tloc, "Expected ';'.");
					}

					rd_tok(rd);
				}
				rd_tok(rd);
			}
			else if(rd->tok == ';')
				rd_tok(rd);
			else
				loc_err(rd->tloc, "Expected ';' or '{'.");
		}
		else
			loc_err(rd->tloc, "Expected assignment or rule.");
	}
	else if(rd->tok == TOK_FOR) {
		char *id;
		struct imm_t *imm;
		struct ast_stmt_t *body;
		struct loc_t loc = rd->tloc;

		if(rd_tok(rd) != TOK_STR)
			loc_err(rd->tloc, "Expected variable name.");

		id = strdup(rd->str);
		if(rd_tok(rd) != ':')
			loc_err(rd->tloc, "Expected ':'.");

		rd_tok(rd);
		imm = rd_imm(rd);
		body = rd_stmt(rd);

		tag = loop_v;
		data.loop = loop_new(id, imm, body, loc);
	}
	else if(rd->tok == TOK_PRINT) {
		tag = print_v;
		rd_tok(rd);
		data.print = print_new(rd_imm(rd));
		if(rd->tok != ';')
			loc_err(rd->tloc, "Expected ';'.");

		rd_tok(rd);
	}
	else if(rd->tok == TOK_MKDEP) {
		struct imm_t *imm;

		rd_tok(rd);
		imm = rd_imm(rd);
		if(rd->tok != ';')
			loc_err(rd->tloc, "Expected ';'.");

		rd_tok(rd);
		return ast_stmt_mkdep(ast_mkdep_new(imm, loc), loc);
	}
	else if((rd->tok == TOK_INCLUDE) || (rd->tok == TOK_IMPORT)) {
		bool nest, opt;
		struct imm_t *imm;

		nest = (rd->tok == TOK_IMPORT);

		rd_tok(rd);
		if((opt = (rd->tok == '?')) == true) 
			rd_tok(rd);

		imm = rd_imm(rd);
		if(rd->tok != ';')
			loc_err(rd->tloc, "Expected ';'.");

		rd_tok(rd);
		return ast_stmt_inc(ast_inc_new(nest, opt, imm), loc);
	}
	else if(rd->tok == '{') {
		tag = block_v;
		data.block = rd_block(rd);
	}
	else
		loc_err(rd->tloc, "Expected statement.", rd->tok, rd->tok);

	return stmt_new(tag, data, loc);
}

/**
 * Read an immediate value.
 *   @rd: The reader.
 *   &returns: The list.
 */
struct imm_t *rd_imm(struct rd_t *rd)
{
	struct imm_t *imm;
	struct raw_t **raw;

	imm = imm_new();
	raw = &imm->raw;

	while((*raw = rd_raw(rd)) != NULL)
		raw = &(*raw)->next;

	return imm;
}

/**
 * Read a raw string.
 *   @rd: The reader.
 *   &returns: The raw string if found, null otherwise.
 */
struct raw_t *rd_raw(struct rd_t *rd)
{
	struct raw_t *raw;

	if((rd->tok != TOK_STR) && (rd->tok != TOK_SPEC) && (rd->tok != TOK_VAR))
		return NULL;

	raw = raw_new(rd->tok == TOK_SPEC, rd->tok == TOK_VAR, strdup(rd->str), rd->tloc);
	rd_tok(rd);

	return raw;
}
