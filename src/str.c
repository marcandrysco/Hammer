#include "inc.h"


/**
 * Get a quoted or plain string from a string.
 *   @str: Ref. The input string.
 *   @loc: The location for error reporting.
 *   &returns: The parsed string.
 */
char *get_str(const char **str, struct loc_t loc)
{
	struct buf_t ret;

	ret = buf_new(32);

	if(**str == '\'') {
		fatal("stub");
	}
	else if(**str == '"') {
		fatal("stub");
	}
	else if(ch_str(**str)) {
		do
			buf_ch(&ret, *(*str)++);
		while(ch_str(**str));
	}
	else
		loc_err(loc, "Expected value.");

	return buf_done(&ret);
}

/**
 * Get a variable name from a string.
 *   @str: The string.
 *   @var: Out. The variable name. Maximum of 255 characters plus a null byte.
 *   @loc: The location for error reporting.
 */
void get_var(const char **str, char *var, struct loc_t loc)
{
	const char *orig = *str;

	if((**str == '@') || (**str == '^') || (**str == '<') || (**str == '~')) {
		*var++ = *(*str)++;
		*var = '\0';
	}
	else {
		if(!ch_var(**str))
			loc_err(loc, "Invalid variable name.");

		do {
			if((*str - orig) >= 255)
				loc_err(loc, "Variable name too long.");

			*var++ = *(*str)++;
		} while(ch_var(**str));

		*var = '\0';
	}
}


/**
 * Trim whitespace from a string.
 *   @str: The string reference.
 */
void str_trim(const char **str)
{
	while(ch_space(**str))
		(*str)++;
}

/**
 * Create a formatted string.
 *   @pat: The printf-style pattern.
 */
char *str_fmt(const char *pat, ...)
{
	char *ret;
	va_list args;

	va_start(args, pat);
	ret = malloc(vsnprintf(NULL, 0, pat, args) + 1);
	va_end(args);

	va_start(args, pat);
	vsprintf(ret, pat, args);
	va_end(args);

	return ret;
}

/**
 * Set a string.
 *   @dst: Ref. The destination, freed is non-null.
 *   @src: The string to use.
 */
void str_set(char **dst, char *src)
{
	if(*dst != NULL)
		free(*dst);

	*dst = src;
}

/**
 * Create the final string.
 *   @str: The string.
 *   @dir: The directory.
 */
void str_final(char **str, const char *dir)
{
	char *ptr, *find;
	struct buf_t buf;

	find = strchr(*str, '$');
	if(find == NULL)
		return;

	ptr = *str;
	buf = buf_new(2 * strlen(*str));

	do {
		buf_mem(&buf, ptr, find - ptr);
		find++;
		if(ch_num(find[0]))
			fatal("stub");
		else if(find[0] == '~')
			buf_str(&buf, dir);
		else if(find[0] == '$')
			buf_ch(&buf, '$');

		ptr = find + 1;
		find = strchr(ptr, '$');
	} while(find != NULL);

	buf_str(&buf, ptr);
	str_set(str, buf_done(&buf));
}


/**
 * Create a string.
 *   @init: The intiial size.
 *   &returns: The string.
 */
struct buf_t buf_new(uint32_t init)
{
	return (struct buf_t){ malloc(init), 0, init };
}

/**
 * Delete a buffer.
 *   @buf: The buffer.
 */
void buf_delete(struct buf_t *buf)
{
	free(buf->str);
}

/**
 * Finish a buffered string, returning the allocated null-terminated string.
 *   @buf: The buffer.
 */
char *buf_done(struct buf_t *buf)
{
	buf_ch(buf, '\0');
	return buf->str;
}


/**
 * Add a character to a buffer.
 *   @buf: The buffer.
 *   @ch: The character.
 */
void buf_ch(struct buf_t *buf, char ch)
{
	if(buf->len >= buf->max)
		buf->str = realloc(buf->str, buf->max *= 2);

	buf->str[buf->len++] = ch;
}

/**
 * Add memory contents to a buffer.
 *   @buf: The buffer.
 *   @mem: The memory.
 *   @len: The length in bytes.
 */
void buf_mem(struct buf_t *buf, const char *mem, uint32_t len)
{
	while(len-- > 0)
		buf_ch(buf, *mem++);
}

/**
 * Add a string to a buffer.
 *   @buf: The buffer.
 *   @str: The string.
 */
void buf_str(struct buf_t *buf, const char *str)
{
	while(*str != '\0')
		buf_ch(buf, *str++);
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
 * Determine if a character is part of a variable.
 *   @ch: The character.
 *   &returns: True if a variable character.
 */
bool ch_var(int ch)
{
	return ch_alnum(ch) || ch == '_';
}

/**
 * Determine if a character is part of a plain string.
 *   @ch: The character.
 *   &returns: True if a variable character.
 */
bool ch_str(int ch)
{
	return ch_alnum(ch) || (strchr("~/._-+=%", ch) != NULL);
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
