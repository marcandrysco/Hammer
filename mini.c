#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
 * local declarations
 */
void quote1(void);
void quote2(void);

void wr(int ch);

void init(char ***set, uint32_t *len);
void push(char ***set, uint32_t *len, char *add);

__attribute__((noreturn)) void error(const char *fmt, ...);

const char *appname;
int ch, last;
FILE *in, *out;


void line(FILE *file)
{
	char ch;

	do
		ch = fgetc(file);
	while((ch >= 0) && (ch != '\n'));
}

void block(void)
{
	char ch;

	ch = fgetc(in);
	while(ch >= 0) {
		while(ch == '*') {
			if((ch = fgetc(in)) == '/')
				return;
		}

		ch = fgetc(in);
	}
}

void norm(void)
{
	while((ch = fgetc(in)) >= 0) {
		if(isspace(ch)) {
			if(!isspace(last))
				wr(ch);

			do
				ch = fgetc(in);
			while(isspace(ch));
		}

		if(ch == '/') {
			ch = fgetc(in);
			if(ch == '/')
				line(in);
			else if(ch == '*')
				block();
			else
				wr('/'), wr(ch);
		}
		else if(ch == '\'')
			quote1();
		else if(ch == '"')
			quote2();
		else
			wr(ch);
	}
}


/**
 * Main entry point.
 *   @argc: The number of argument.s
 *   @argv: The arrray of arguments.
 */
int main(int argc, char **argv) {
	appname = argv[0];

	in = stdin;
	out = stdout;

	last = '\n';
	norm();

	return 0;
}


/**
 * Handle a single quote.
 */
void quote1(void)
{
	char ch;

	wr('\'');

	while((ch = fgetc(in)) != '\'') {
		if(ch == '\\') {
			wr('\\');
			ch = fgetc(in);
		}
		else if(ch < 0)
			break;

		wr(ch);
	}

	wr('\'');
}

/**
 * Handle a double quote.
 */
void quote2(void)
{
	char ch;

	wr('"');

	while((ch = fgetc(in)) != '"') {
		if(ch == '\\') {
			wr('\\');
			ch = fgetc(in);
		}
		else if(ch < 0)
			break;

		wr(ch);
	}

	wr('"');
}


/**
 * Write a character.
 *   @ch: The character.
 */
void wr(int ch)
{
	if(ch >= 0)
		fputc(last = ch, out);
}


/**
 * Initialize an set.
 *   @set: The set.
 *   @len: The length.
 */
void init(char ***set, uint32_t *len)
{
	*set = malloc(1);
	*len = 0;
}

/**
 * Push a string onto a set.
 *   @set: The set reference.
 *   @len: The length.
 *   @add: The string to add.
 */
void push(char ***set, uint32_t *len, char *add)
{
	*set = realloc(*set, (*len + 1) * sizeof(char **));
	(*set)[(*len)++] = add;
}


/**
 * Report an error.
 *   @fmt: The printf-style format string.
 *   @...: The printf-style arugments.
 *   &noreturn
 */
void error(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	fprintf(stderr, "%s: ", appname);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	exit(1);
}
