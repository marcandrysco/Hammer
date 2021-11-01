#include "inc.h"


/**
 * Create a block.
 *   &returns: The block.
 */
struct block_t *block_new(void)
{
	struct block_t *block;

	block = malloc(sizeof(struct block_t));
	block->stmt = NULL;

	return block;
}

/**
 * Delete a block.
 *   @block: The block.
 */
void block_delete(struct block_t *block)
{
	stmt_clear(block->stmt);
	free(block);
}


/**
 * Create a syntatic rule.
 *   @gen: Consumed. The generator immediate.
 *   @dep: Consumed. The generator immediate.
 *   @loc: The location.
 *   &returns: The syntatic rule.
 */
struct syn_t *syn_new(struct imm_t *gen, struct imm_t *dep, struct loc_t loc)
{
	struct syn_t *syn;

	syn = malloc(sizeof(struct syn_t));
	syn->gen = gen;
	syn->dep = dep;
	syn->loc = loc;
	syn->cmd = list_new((del_f)imm_delete);

	return syn;
}

/**
 * Delete a syntatic rule.
 *   @syn: The syntatic rule.
 */
void syn_delete(struct syn_t *syn)
{
	imm_delete(syn->gen);
	imm_delete(syn->dep);
	list_delete(syn->cmd);
	free(syn);
}


/**
 * Add a command to the syntatic rule.
 *   @syn: The syntax.
 *   @cmd: The command.
 */
void syn_add(struct syn_t *syn, struct imm_t *cmd)
{
	list_add(syn->cmd, cmd);
}


/**
 * Create a directory statement.
 *   @def: The default flag.
 *   @imm: The immediate value.
 *   @block: Optional. The configuration block.
 *   &returns: The directory statement.
 */
struct dir_t *dir_new(bool def, struct raw_t *raw, struct block_t *block)
{
	struct dir_t *dir;

	dir = malloc(sizeof(struct dir_t));
	*dir = (struct dir_t){ def, raw, block };
	
	return dir;
}

/**
 * Delete a directory statement.
 *   @dir: The directory statement.
 */
void dir_delete(struct dir_t *dir)
{
	if(dir->block != NULL)
		block_delete(dir->block);

	raw_delete(dir->raw);
	free(dir);
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
 * Create a statement.
 *   @tag: The tag.
 *   @data: Consumed. The data.
 *   @loc: The location.
 *   &returns: The statement.
 */
struct stmt_t *stmt_new(enum stmt_e tag, union stmt_u data, struct loc_t loc)
{
	struct stmt_t *stmt;

	stmt = malloc(sizeof(struct stmt_t));
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
void stmt_delete(struct stmt_t *stmt)
{
	switch(stmt->tag) {
	case assign_v: assign_delete(stmt->data.assign); break;
	case syn_v: syn_delete(stmt->data.syn); break;
	case dir_v: dir_delete(stmt->data.dir); break;
	case print_v: print_delete(stmt->data.print); break;
	}

	free(stmt);
}

/**
 * Clear a list of statement.
 *   @stmt: The statement list.
 */
void stmt_clear(struct stmt_t *stmt)
{
	struct stmt_t *tmp;

	while(stmt != NULL) {
		stmt = (tmp = stmt)->next;
		stmt_delete(tmp);
	}
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
 * Create an assignment.
 *   @id: Consumed. The identifier.
 *   @val: Cosnumed. The value.
 *   &returns: The assignment.
 */
struct assign_t *assign_new(struct raw_t *id, struct imm_t *val)
{
	struct assign_t *assign;

	assign = malloc(sizeof(struct assign_t));
	assign->id = id;
	assign->val = val;

	return assign;
}

/**
 * Delete an assignment.
 *   @assign: The assignment.
 */
void assign_delete(struct assign_t *assign)
{
	raw_delete(assign->id);
	imm_delete(assign->val);
	free(assign);
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
u32 imm_len(struct imm_t *imm)
{
	u32 n;
	struct raw_t *raw;

	n = 0;
	for(raw = imm->raw; raw != NULL; raw = raw->next)
		n++;

	return n;
}


/**
 * Create an raw string.
 *   @spec: Special value flag.
 *   @quote: Quoted flag.
 *   @str: Consumed. The string.
 *   @loc: The location.
 *   &returns: The raw string.
 */
struct raw_t *raw_new(bool spec, bool quote, char *str, struct loc_t loc)
{
	struct raw_t *raw;

	raw = malloc(sizeof(struct raw_t));
	raw->spec = spec;
	raw->quote = quote;
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
	return raw_new(raw->spec, raw->quote, strdup(raw->str), raw->loc);
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
struct read_t {
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
#define rd_ch rd_ch0
#define rd_tok rd_tok0
#define rd_top rd_top0
int rd_ch(struct read_t *rd);
int rd_tok(struct read_t *rd);

void rd_reset(struct read_t *rd);
int rd_push(struct read_t *rd, char ch);
int rd_buf(struct read_t *rd);

struct block_t *rd_top(struct read_t *rd);
struct stmt_t *rd_stmt(struct read_t *rd);
struct imm_t *rd_imm(struct read_t *rd);
struct raw_t *rd_raw(struct read_t *rd);


/**
 * Load a hammer file from the top.
 *   @path: The path.
 *   &returns: The block.
 */
struct block_t *ham_load(const char *path)
{
	struct read_t rd;
	struct block_t *block;

	rd.file = fopen(path, "r");
	if(rd.file == NULL)
		cli_err("Cannot open '%s'.", path);

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
int rd_ch(struct read_t *rd)
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
void rd_reset(struct read_t *rd)
{
	rd->len = 0;
}

/**
 * Push a character onto the reader's string buffer.
 *   @rd: The reader.
 *   @ch: The character.
 *   &returns: The next character.
 */
int rd_push(struct read_t *rd, char ch)
{
	if(rd->len >= rd->max)
		rd->str = realloc(rd->str, rd->max *= 2);

	rd->str[rd->len++] = ch;

	return rd->ch;
}

/**
 * Buffer the next characte from the reader and advance.
 *   @rd: The reader.
 *   &returns: The next character.
 */
int rd_buf(struct read_t *rd)
{
	rd_push(rd, rd->ch);

	return rd_ch(rd);
}

struct sym_t {
	int tok;
	const char *str;
};

#undef TOK_STR
#undef TOK_EOF
#define TOK_STR   0x1000
#define TOK_QUOTE 0x1001
#define TOK_VAR   0x1002
#define TOK_SPEC  0x1003
#define TOK_DIR   0x2000
#define TOK_FOR   0x2001
#define TOK_IF    0x2002
#define TOK_PRINT 0x2003
#define TOK_DEF   0x2004
#define TOK_EOF   0x7FFF

struct sym_t syms[] = {
	{ '{', "{" },
	{ '}', "}" },
	{ ':', ":" },
	{ ';', ";" },
	{ '=', "=" },
	{ 0,   NULL }
};

struct sym_t keys[] = {
	{ TOK_DIR,   "dir" },
	{ TOK_FOR,   "for" },
	{ TOK_IF,    "if" },
	{ TOK_PRINT, "print" },
	{ TOK_DEF  , "default" },
	{ 0,         NULL }
};


/**
 * Read the next token.
 *   @rd: The reader.
 *   &returns: The token.
 */
int rd_tok(struct read_t *rd)
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

		return rd->tok = sym->tok;
	}

	if(ch_str(rd->ch)) {
		do
			rd_buf(rd);
		while(ch_str(rd->ch));

		rd_push(rd, '\0');

		for(sym = keys; sym->tok != 0; sym++) {
			if(strcmp(rd->str, sym->str) == 0)
				return rd->tok = sym->tok;
		}

		rd->tok = (rd->str[0] == '.') ? TOK_SPEC : TOK_STR;
	}
	else if((rd->ch == '"') || (rd->ch == '\'')) {
		char delim = rd->ch;

		rd_ch(rd);

		for(;;) {
			if(rd->ch == '\\') {
				char ch;

				switch(rd_ch(rd)) {
				case 't': ch = '\t'; break;
				case 'n': ch = '\n'; break;
				default: loc_err(rd->tloc, "Invalid escape character '\\%c'.", rd->ch);
				}

				rd_push(rd, ch);
				rd_ch(rd);
			}
			else if(rd->ch == TOK_EOF)
				loc_err(rd->tloc, "Unexpected end-of-file");
			else if(rd->ch == '\n')
				loc_err(rd->tloc, "Unexpected newline.");
			else if(rd->ch == delim)
				break;
			else
				rd_buf(rd);
		}

		rd->tok = TOK_QUOTE;
		rd_ch(rd);
		rd_push(rd, '\0');
	}
	else if(rd->ch < 0)
		rd->tok = TOK_EOF;
	else
		fatal("FIXME boo %c", rd->ch);

	return rd->tok;
}


/**
 * Read a block.
 *   @rd: The reader.
 *   &returns: The block.
 */
struct block_t *rd_block(struct read_t *rd)
{
	struct block_t *block;
	struct stmt_t **stmt;

	if(rd->tok != '{')
		loc_err(rd->tloc, "Expected ';' or '{'.");

	rd_tok(rd);
	block = block_new();
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
struct block_t *rd_top(struct read_t *rd)
{
	struct block_t *block;
	struct stmt_t **stmt;

	block = block_new();
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
struct stmt_t *rd_stmt(struct read_t *rd)
{
	struct loc_t loc;
	enum stmt_e tag;
	union stmt_u data;

	loc = rd->loc;
	if((rd->tok == TOK_STR) || (rd->tok == TOK_QUOTE) || (rd->tok == TOK_SPEC)) {
		struct imm_t *lhs, *rhs;

		lhs = rd_imm(rd);

		if(rd->tok == '=') {
			if(imm_len(lhs) == 0)
				loc_err(rd->tloc, "Missing variable name.");
			else if(imm_len(lhs) >= 2)
				loc_err(rd->tloc, "Invalid variable name.");

			rd_tok(rd);
			rhs = rd_imm(rd);

			tag = assign_v;
			data.assign = assign_new(raw_dup(lhs->raw), rhs);

			if(rd->tok != ';')
				loc_err(rd->tloc, "Expected ';'.");

			rd_tok(rd);
			imm_delete(lhs);
		}
		else if(rd->tok == ':') {
			loc = rd->tloc;
			rd_tok(rd);
			rhs = rd_imm(rd);
	
			tag = syn_v;
			data.syn = syn_new(lhs, rhs, loc);
			if(rd->tok == '{') {
				rd_tok(rd);
				while(rd->tok != '}') {
					list_add(data.syn->cmd, rd_imm(rd));
					if(rd->tok != ';')
						loc_err(rd->tloc, "Expected ';'.");

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
	else if(rd->tok == TOK_DIR) {
		struct raw_t *raw;
		bool def = false;
		struct block_t *block = NULL;

		tag = dir_v;
		rd_tok(rd);
		raw = rd_raw(rd);

		if(rd->tok == TOK_DEF) {
			def = true;
			rd_tok(rd);
		}

		if(rd->tok == '{')
			block = rd_block(rd);
		else if(rd->tok == ';')
			rd_tok(rd);
		else
			loc_err(rd->tloc, "Expected ';' or '{'.");

		data.dir = dir_new(def, raw, block);
	}
	else if(rd->tok == TOK_PRINT) {
		tag = print_v;
		rd_tok(rd);
		data.print = print_new(rd_imm(rd));
		if(rd->tok != ';')
			loc_err(rd->tloc, "Expected ';'.");

		rd_tok(rd);
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
struct imm_t *rd_imm(struct read_t *rd)
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
struct raw_t *rd_raw(struct read_t *rd)
{
	struct raw_t *raw;

	if((rd->tok != TOK_STR) && (rd->tok != TOK_QUOTE) && (rd->tok != TOK_SPEC))
		return NULL;

	raw = raw_new(rd->tok == TOK_SPEC, rd->tok == TOK_QUOTE, strdup(rd->str), rd->tloc);
	rd_tok(rd);

	return raw;
}
