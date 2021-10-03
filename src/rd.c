#include "inc.h"


/**
 * Reader structure.
 *   @file: The file.
 *   @ch: The current character.
 *   @tok: The token list.
 *   @loc: The current location.
 */
struct rd_t {
	FILE *file;
	char ch;
	struct tok_t *tok;

	struct loc_t loc;
};


/*
 * local declarations
 */
static void str_add(char **buf, uint32_t *len, uint32_t *max, char ch);


/**
 * Open a reader.
 *   @path: The path.
 *   &returns: The reader.
 */
struct rd_t *rd_open(const char *path)
{
	struct rd_t *rd;

	rd = malloc(sizeof(struct rd_t));
	rd->loc.path = path;
	rd->loc.lin = 1;
	rd->loc.col = 0;
	rd->tok = NULL;
	rd->file = fopen(path, "r");
	if(rd->file == NULL)
		cli_err("Failed to open '%s'. %s.", path, strerror(errno));

	rd_ch(rd);
	rd->tok = tok_new(TOK_EOF, strdup(""), rd->loc);

	return rd;
}

/**
 * Close the reader.
 *   @rd: The reader.
 */
void rd_close(struct rd_t *rd)
{
	struct tok_t *tok;

	while(rd->tok != NULL) {
		rd->tok = (tok = rd->tok)->next;
		tok_delete(tok);
	}

	fclose(rd->file);
	free(rd);
}


struct tok_t *rd_tok(struct rd_t *rd);

/**
 * Read a character.
 *   @rd: The reader.
 *   &returns: The next character.
 */
char rd_ch(struct rd_t *rd)
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
 * Read a token.
 *   @rd: The reader.
 *   &returns: The token.
 */
struct tok_t *rd_tok(struct rd_t *rd)
{
	struct loc_t loc;
	struct tok_t **tok;

	for(;;) {
		while(ch_space(rd->ch))
			rd_ch(rd);

		if(rd->ch != '#')
			break;

		while((rd->ch != '\n') && (rd->ch > 0))
			rd_ch(rd);
	}

	tok = &rd->tok;
	while(*tok != NULL)
		tok = &(*tok)->next;

	loc = rd->loc;

	if(rd->ch == EOF)
		*tok = tok_new(TOK_EOF, strdup(""), loc);
	else if(rd->ch == '\'') {
		char add, *str = malloc(32);
		uint32_t len = 0, max = 32;

		while(rd_ch(rd) != '\'') {
			if((rd->ch == '\n') || (rd->ch < 0))
				loc_err(rd->loc, "Unterminated string.");

			if(rd->ch == '\\') {
				switch(rd_ch(rd)) {
				case '\\': add = '\\'; break;
				case '0': add = '\0'; break;
				case '\'': add = '\''; break;
				case '"': add = '\"'; break;
				case 't': add = '\t'; break;
				case 'v': add = '\v'; break;
				case 'r': add = '\r'; break;
				case 'n': add = '\n'; break;
				default: loc_err(rd->loc, "Unknown escape '\\%c'.", rd->ch);
				}
			}
			else
				add = rd->ch;

			str_add(&str, &len, &max, add);
		}

		rd_ch(rd);
		str_add(&str, &len, &max, '\0');
		str = realloc(str, len);

		*tok = tok_new(TOK_STR, str, loc);
	}
	else if(rd->ch == '"') {
		char add, *str = malloc(32);
		uint32_t len = 0, max = 32;

		while(rd_ch(rd) != '"') {
			if((rd->ch == '\n') || (rd->ch < 0))
				loc_err(rd->loc, "Unterminated string.");

			if(rd->ch == '\\') {
				switch(rd_ch(rd)) {
				case '\\': add = '\\'; break;
				case '0': add = '\0'; break;
				case '\'': add = '\''; break;
				case '"': add = '\"'; break;
				case 't': add = '\t'; break;
				case 'v': add = '\v'; break;
				case 'r': add = '\r'; break;
				case 'n': add = '\n'; break;
				case '$': add = '$'; break;
				default: loc_err(rd->loc, "Unknown escape '\\%c'.", rd->ch);
				}

				str_add(&str, &len, &max, add);
			}
			else
				str_add(&str, &len, &max, rd->ch);
		}

		rd_ch(rd);
		str_add(&str, &len, &max, '\0');
		str = realloc(str, len);

		*tok = tok_new(TOK_STR, str, loc);
	}
	else if(ch_id(rd->ch)) {
		char *str = malloc(32);
		uint32_t len = 0, max = 32;

		do
			str_add(&str, &len, &max, rd->ch);
		while(ch_id(rd_ch(rd)));

		str_add(&str, &len, &max, '\0');
		str = realloc(str, len);

		*tok = tok_new(TOK_ID, str, loc);
	}
	else {
		uint32_t id = 0;
		const char *str;

		switch(rd->ch) {
		case '{': id = '{'; str = "{"; rd_ch(rd); break;
		case '}': id = '}'; str = "}"; rd_ch(rd); break;
		case '=': id = '='; str = "="; rd_ch(rd); break;
		case ':': id = ':'; str = ":"; rd_ch(rd); break;
		case ';': id = ';'; str = ";"; rd_ch(rd); break;
		}

		if(id == 0)
			rd_err(rd, "Unknown input '%c'.", rd->ch);

		*tok = tok_new(id, strdup(str), loc);
	}

	return *tok;
}

/**
 * Get the top (current) token.
 *   @rd: The reader.
 *   &returns: The token.
 */
struct tok_t *rd_top(struct rd_t *rd)
{
	return rd_get(rd, 0);
}

/**
 * Retrieve the `idx`th token from the reader.
 *   @rd: The reader.
 *   @idx: The index.
 *   &returns: The token.
 */
struct tok_t *rd_get(struct rd_t *rd, int idx)
{
	struct tok_t **tok = &rd->tok;

	idx = (idx < 0) ? 0 : (idx + 1);

	for(;;) {
		if(*tok == NULL)
			*tok = rd_tok(rd);

		if(idx-- <= 0)
			return *tok;

		tok = &(*tok)->next;
	}

	return NULL;
}

/**
 * Advance the reader `cnt` tokens.
 *   @rd: The reader.
 *   @cnt: The number of tokens.
 *   &returns: The top (current) token.
 */
struct tok_t *rd_adv(struct rd_t *rd, uint32_t cnt)
{
	struct tok_t *tok;

	while(cnt-- > 0) {
		rd->tok = (tok = rd->tok)->next;
		tok_delete(tok);
	}

	return rd_top(rd);
}

/**
 * Print an error at a token.
 *   @tok: The token.
 *   @fmt: The format string.
 *   @...: The printf-style arguments.
 */
void tok_err(struct tok_t *tok, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	fprintf(stderr, "%s:%u:%u: ", tok->loc.path, tok->loc.lin, tok->loc.col);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);

	exit(1);
}


/**
 * Print out an error at the current character.
 *   @rd: The reader.
 *   @fmt: The printf-style format string.
 *   @...: The printf-style arguments.
 *   @noreturn
 */
void rd_err(struct rd_t *rd, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	fprintf(stderr, "%s:%u:%u: ", rd->loc.path, rd->loc.lin, rd->loc.col);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);

	exit(1);
}


/**
 * Create a new token.
 *   @id: The identifier.
 *   @str: The string buffer.
 *   @loc: The location.
 *   &returns: The token.
 */
struct tok_t *tok_new(uint32_t id, char *str, struct loc_t loc)
{
	struct tok_t *tok;

	tok = malloc(sizeof(struct tok_t));
	*tok = (struct tok_t){ id, str, loc, NULL };

	return tok;
}

/**
 * Delete a token.
 *   @tok: The token.
 */
void tok_delete(struct tok_t *tok)
{
	free(tok->str);
	free(tok);
}


/**
 * Determine if a character is a space.
 *   @ch: The character.
 *   &returns: True if space.
 */
bool ch_space(int ch)
{
	return (ch == ' ') || (ch == '\r') || (ch == '\t') || (ch == '\n') || (ch == '\v');
}

/**
 * Determine if a character is an alphabet character.
 *   @ch: The character.
 *   &returns: True if space.
 */
bool ch_alpha(int ch)
{
	return ((ch >= 'A') && (ch <= 'Z')) || ((ch >= 'a') && (ch <= 'z'));
}

/**
 * Determine if a character is a numeric character.
 *   @ch: The character.
 *   &returns: True if space.
 */
bool ch_num(int ch)
{
	return (ch >= '0') && (ch <= '9');
}

/**
 * Determine if a character is an alphabet or numeric character.
 *   @ch: The character.
 *   &returns: True if space.
 */
bool ch_alnum(int ch)
{
	return ch_alpha(ch) || ch_num(ch);
}

/**
 * Determine if a character is part of an identifier.
 *   @ch: The character.
 *   &returns: True if an ID character.
 */
bool ch_id(int ch)
{
	return !ch_space(ch) && (ch != '{') && (ch != '}') && (ch != ':') && (ch != ';') && (ch != '=');
}


/**
 * Generate an error at a specific location.
 *   @loc: The location.
 *   @fmt: The printf-style format string.
 *   @...: The printf-style arguments.
 *   @noreturn
 */
void loc_err(struct loc_t loc, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	fprintf(stderr, "%s:%u:%u: ", loc.path, loc.lin, loc.col);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);

	exit(1);
}


/**
 * Add to a string.
 *   @buf: Ref. The buffer.
 *   @len: Ref. The length.
 *   @max: Ref. The buffer maximum length.
 *   @ch: The character.
 */
static void str_add(char **buf, uint32_t *len, uint32_t *max, char ch)
{
	if(*len >= *max)
		*buf = realloc(*buf, *max = 2 * *len);

	(*buf)[(*len)++] = ch;
}
