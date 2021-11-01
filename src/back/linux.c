#include "../inc.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


/*
 * global declarations
 */
i64 os_memcnt = 0;


/**
 * Print to the output.
 *   @fmt: The printf-style format string.
 *   @...: The printf-style arguments.
 */
void print(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

/**
 * Print to the output.
 *   @fmt: The printf-style format string.
 *   @args: The printf-style arguments.
 */
void printv(const char *fmt, va_list args)
{
	vprintf(fmt, args);
}

/**
 * Print an error a terminate the program.
 *   @fmt: The format
 *   @fmt: The printf-style format string.
 *   @...: The printf-style arguments.
 *   &noreturn
 */
void fatal(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");

	abort(); //FIXME change to exit before release
}


#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * Execute a command based off of a value.
 *   @val: The value.
 */
void os_exec(struct val_t *val)
{
	pid_t pid;
	uint32_t i, n;
	char **args;

	n = val_len(val);
	args = malloc(sizeof(void *) * (n + 1));
	for(i = 0; i < n; i++) {
		args[i] = strdup(val->str);

		print("%s%s", val->str, (i < (n-1)) ? " " : "\n");
		val = val->next;
	}
	args[n] = NULL;

	pid = vfork();
	if(pid == 0)
		execvp(args[0], args);

	int stat;
	wait(&stat);

	for(i = 0; i < n; i++)
		free(args[i]);

	free(args);
}

/**
 * Retrieve the modifiation time for a file path.
 *   @path: The file path.
 *   &returns: The time in microseconds.
 */
int64_t os_mtime(const char *path)
{
	struct stat info;

	if(stat(path, &info) < 0)
		return INT64_MIN;

	return 1000000 * info.st_mtim.tv_sec + info.st_mtim.tv_nsec / 1000;
}

void os_mkdir(const char *path)
{
	mkdir(path, 0777);
}
