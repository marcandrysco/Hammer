#include "inc.h"

/*
 * variables
 */
char *cli_app = NULL;


void arr_init(const char ***arr, uint32_t *cnt) {
	*arr = malloc(0);
	*cnt = 0;
}

void arr_delete(const char **arr, uint32_t cnt) {
	free(arr);
}

void arr_add(const char ***arr, uint32_t *cnt, const char *str)
{
	*arr = realloc(*arr, (*cnt + 1) * sizeof(const char *));
	(*arr)[(*cnt)++] = str;
}


/**
 * Process the arguments.
 *   @args: The arguments.
 */
void cli_proc(char **args)
{
	struct rd_t *rd;
	struct ctx_t *ctx;
	struct ns_t *ns;
	const char **arr;
	uint32_t i, cnt;

	arr_init(&arr, &cnt);

	for(i = 0; args[i] != NULL; i++)
		arr_add(&arr, &cnt, args[i]);

	arr_add(&arr, &cnt, NULL);

	rd = rd_open("Hammer");
	ctx = ctx_new();
	ns = ns_new(NULL, NULL);

	par_top(rd, ctx, ns);
	ctx_run(ctx, arr);

	ns_delete(ns);
	ctx_delete(ctx);
	rd_close(rd);
	arr_delete(arr, cnt);
}

/**
 * Print an error and exit.
 *   @fmt: The printf-style format .
 *   @...: The printf-style arguments.
 *   &noreturn
 */
void cli_err(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	fprintf(stderr, "%s: ", cli_app);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");

	exit(1);
}
