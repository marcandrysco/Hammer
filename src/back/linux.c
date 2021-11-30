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

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * Execute a command based off of a value.
 *   @cmd: The command.
 */
void os_exec(struct cmd_t *cmd)
{
	int in, out;
	pid_t pid;
	uint32_t i, n;
	char **args;

	n = val_len(cmd->val);
	args = malloc(sizeof(void *) * (n + 1));
	for(i = 0; i < n; i++) {
		args[i] = strdup(cmd->val->str);

		print("%s%s", cmd->val->str, (i < (n-1)) ? " " : "");
		cmd->val = cmd->val->next;
	}
	args[n] = NULL;

	if(cmd->in) {
		print(" < %s", cmd->in);
		in = open(cmd->in, O_RDONLY);
		if(in < 0)
			fatal("Cannot open '%s' for reading. %s.", in, strerror(errno));
	}
	else
		in = -1;

	if(cmd->out) {
		print(" %s %s", cmd->append ? ">>" : ">", cmd->out);
		out = open(cmd->out, O_WRONLY | O_CREAT | (cmd->append ? O_APPEND : 0), 0644);
		if(out < 0)
			fatal("Cannot open '%s' for writing . %s.", out, strerror(errno));
	}
	else
		out = -1;

	print("\n");
	pid = vfork();
	if(pid == 0) {
		if(in >= 0) {
			dup2(in, STDIN_FILENO);
			close(in);
		}

		if(out >= 0) {
			dup2(out, STDOUT_FILENO);
			close(out);
		}

		execvp(args[0], args);
	}

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
