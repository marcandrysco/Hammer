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
	struct block_t *top;
	struct opt_t opt;
	struct ctx_t *ctx;
	const char **arr;
	uint32_t i, k, cnt;

	opt.force = false;
	opt.dir = NULL;

	arr_init(&arr, &cnt);

	for(i = 0; args[i] != NULL; i++) {
		if(args[i][0] == '-') {
			if(args[i][1] == '-') {
				fatal("FIXME stub");
			}
			else {
				k = 1;

				while(args[i][k] != '\0') {
					switch(args[i][k]) {
					case 'f': opt.force = true; break;

					case 'd':
						if(opt.dir != NULL)
							cli_err("Directory already given.");
						else if(args[i][k + 1] == '\0') {
							if((opt.dir = args[++i]) == NULL)
								cli_err("Missing directory (-d).");
						}
						else
							opt.dir = args[i] + k + 1;

						k = strlen(args[i]);
						continue;

					default: cli_err("Unknown option '-%c'.", args[i][k]); 
					}

					k++;
				}
			}
		}
		else
			arr_add(&arr, &cnt, args[i]);
	}

	arr_add(&arr, &cnt, NULL);

	top = ham_load("Hammer");
	ctx = ctx_new(&opt);

	eval_top(top, ctx);
	ctx_run(ctx, arr);

	block_delete(top);
	ctx_delete(ctx);
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
