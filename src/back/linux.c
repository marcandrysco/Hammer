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
 * Initialize the OS backend.
 */
void os_init(void)
{
	setlinebuf(stdout);
}

struct os_job_t {
	int *pid;

	uint32_t len;
};

/**
 * Execute a command based off of a value.
 *   @cmd: The command.
 */
int os_exec(struct cmd_t *cmd)
{
	uint32_t i, n;
	char **args;
	struct val_t *val;
	struct rt_pipe_t *iter;
	pid_t pid = 0;
	int in = -1, out = -1, pair[2];

	for(iter = cmd->pipe; iter != NULL; iter = iter->next) {
		val = iter->cmd;

		n = val_len(val);
		args = malloc(sizeof(void *) * (n + 1));
		for(i = 0; i < n; i++) {
			args[i] = strdup(val->str);
			val = val->next;
		}
		args[n] = NULL;

		if(cmd->in && (iter == cmd->pipe)) {
			in = open(cmd->in, O_RDONLY);
			if(in < 0)
				fatal("Cannot open '%s' for reading. %s.", in, strerror(errno));
		}
		else if(iter != cmd->pipe)
			in = pair[0];
		else
			in = -1;

		if(cmd->out && (iter->next == NULL)) {
			out = open(cmd->out, O_WRONLY | O_CREAT | (cmd->append ? O_APPEND : O_TRUNC), 0644);
			if(out < 0)
				fatal("Cannot open '%s' for writing . %s.", out, strerror(errno));
		}
		else if(iter->next != NULL) {
			if(pipe(pair) < 0)
				fatal("Cannot create pipe. %s.", strerror(errno));

			out = pair[1];
		}
		else
			out = -1;

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

		if(in >= 0)
			close(in);

		if(out >= 0)
			close(out);

		for(i = 0; i < n; i++)
			free(args[i]);

		free(args);
	}

	return pid;
}

/**
 * Wait for a child to exit.
 */
int os_wait(void)
{
	int pid, stat;

	for(;;) {
		pid = wait(&stat);
		if(pid >= 0)
			break;

		if(errno != EINTR)
			fatal("Failed to wait. %s.", strerror(errno));
	}

	stat = WEXITSTATUS(stat);
	if(stat != 0)
		fatal("Command terminated with status %d.", stat);

	return pid;
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
