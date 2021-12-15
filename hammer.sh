#!/bin/sh

## 
# Hammer build system
#   version 1.0dev0
##

TMP=".hammer.c"
BIN="hammer.cache"
CC="gcc"

test $VERBOSE && echo "cc=$CC"

sed '0,/\#\#csrc\#\#/d' "$0" > "$TMP" || exit $?
"$CC" -Wall -g -O2 "$TMP" -o "$BIN" || exit $?

exit 0
##csrc##

/*
 * required headers
 */
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * shorter type names
 */
typedef int64_t i64;
typedef uint64_t u64;

/*
 * structure prototypes
 */
struct ast_cmd_t;
struct ast_pipe_t;
struct cmd_t;
struct ctx_t;
struct env_t;
struct list_t;
struct queue_t;
struct ns_t;
struct rd_t;
struct rule_t;
struct rt_pipe_t;
struct seq_t;
struct set_t;
struct target_t;
struct target_list_t;
struct val_t;

struct loc_t;
struct tok_t;
struct val_t;

typedef int(*cmp_f)(const void *, const void *);
typedef void(*del_f)(void *);

typedef struct val_t *(func_t)(struct val_t **args, uint32_t cnt, struct loc_t loc);

/*
 * common declarations
 */
uint64_t hash64(uint64_t hash, const char *str);

/*
 * base declarations
 */
struct block_t *ham_load(const char *path);

/*
 * backend declarations
 */
extern i64 os_memcnt;

void print(const char *fmt, ...);
void printv(const char *fmt, va_list args);
void fatal(const char *fmt, ...) __attribute__((noreturn));

void os_init(void);
int os_exec(struct cmd_t *cmd);
int os_wait(void);
int64_t os_mtime(const char *path);
void os_mkdir(const char *path);

/*
 * argument declarations
 */
void args_init(struct val_t ***args, uint32_t *cnt);
void args_add(struct val_t ***args, uint32_t *cnt, struct val_t *val);
void args_delete(struct val_t **args, uint32_t cnt);

/*
 * set declarations
 */
struct set_t *set_new(void);
void set_delete(struct set_t *set, void(*del)(void *));

void set_add(struct set_t *set, void *ref);


/**
 * List structure.
 *   @head, tail: The head and tail links.
 *   @del: Value deletion function.
 */
struct list_t {
	struct link_t *head, **tail;
	void (*del)(void *);
};

/**
 * Link structure.
 *   @val: The value.
 *   @next: The next link.
 */
struct link_t {
	void *val;
	struct link_t *next;
};

/*
 * list declarations
 */
struct list_t *list_new(void(*del)(void *));
void list_delete(struct list_t *list);

void list_add(struct list_t *list, void *val);


/**
 * Options structure.
 *   @force: Force rebuild.
 *   @jobs: The number of jobs.
 *   @dir: The selected directory.
 */
struct opt_t {
	bool force;
	int jobs;
	const char *dir;
};

/**
 * Context structure.
 *   @opt: The options.
 *   @map: The target map.
 *   @rules: The set of rules.
 *   @gen, dep: The generator and depedency values.
 *   @dir: The selected directory.
 *   @gen, deps: The generated and dependency targets.
 */
struct ctx_t {
	const struct opt_t *opt;

	struct map_t *map;
	struct rule_list_t *rules;

	struct val_t *gen, *dep;
	struct bind_t *dir;

	struct target_list_t *gens, *deps;
};

/*
 * context declarations
 */
struct ctx_t *ctx_new(const struct opt_t *opt);
void ctx_delete(struct ctx_t *ctx);

void ctx_run(struct ctx_t *ctx, const char **builds);

struct target_t *ctx_target(struct ctx_t *ctx, bool spec, const char *path);
struct rule_t *ctx_rule(struct ctx_t *ctx, const char *id, struct target_list_t *gens, struct target_list_t *deps, struct seq_t *seq);


/**
 * Rule structure.
 *   @id: The identifier.
 *   @gens, deps: The generated an depdency targets.
 *   @seq: The command sequence.
 *   @add: Flag indicated it has been added.
 *   @edges: The unresolved edge count.
 */
struct rule_t {
	char *id;
	struct target_list_t *gens, *deps;
	struct seq_t *seq;

	bool add;
	uint32_t edges;
};

/**
 * Rule list structure.
 *   @inst: The instance list.
 */
struct rule_list_t {
	struct rule_inst_t *inst;
};

/**
 * Rule instance structure.
 *   @rule: The rule.
 *   @next: The next instance.
 */
struct rule_inst_t {
	struct rule_t *rule;
	struct rule_inst_t *next;
};

/**
 * Rule iterator stucture.
 *   @inst: The current instance.
 */
struct rule_iter_t {
	struct rule_inst_t *inst;
};

/*
 * rule declarations
 */
struct rule_t *rule_new(char *id, struct target_list_t *gens, struct target_list_t *deps, struct seq_t *seq);
void rule_delete(struct rule_t *rule);

/*
 * rule iterator declarations
 */
struct rule_iter_t rule_iter(struct rule_list_t *list);
struct rule_t *rule_next(struct rule_iter_t *iter);

/*
 * rule list declarations
 */
struct rule_list_t *rule_list_new(void);
void rule_list_delete(struct rule_list_t *list);

void rule_list_add(struct rule_list_t *list, struct rule_t *rule);

/*
 * queue declarations
 */
struct queue_t *queue_new(void);
void queue_delete(struct queue_t *queue);

void queue_recur(struct queue_t *queue, struct rule_t *rule);
void queue_add(struct queue_t *queue, struct rule_t *rule);
struct rule_t *queue_rem(struct queue_t *queue);


/**
 * Target structure.
 *   @path: The path.
 *   @flags: The flags.
 *   @mtime: The modification time.
 *   @rule: The associated rule.
 *   @edge: The forward edges.
 */
struct target_t {
	char *path;
	uint32_t flags;
	int64_t mtime;

	struct rule_t *rule;
	struct edge_t *edge;
};

/**
 * Target list structure.
 *   @inst: The head instance.
 */
struct target_list_t {
	struct target_inst_t *inst;
};

/**
 * Target instance structure.
 *   @target: The target.
 *   @next: The next instance.
 */
struct target_inst_t {
	struct target_t *target;
	struct target_inst_t *next;
};

/**
 * Target iterator stucture.
 *   @inst: The current instance.
 */
struct target_iter_t {
	struct target_inst_t *inst;
};

/**
 * Edge instance stucture.
 *   @rule: The target rule.
 *   @next: The next instance.
 */
struct edge_t {
	struct rule_t *rule;
	struct edge_t *next;
};

/**
 * Flag definitions
 *   @FLAG_BUILD: Built target (not source).
 *   @FLAG_SPEC: Special rule.
 */
#define FLAG_BUILD (1 << 0)
#define FLAG_SPEC  (1 << 1)

/*
 * target declarations
 */
struct target_t *target_new(bool spec, char *path);
void target_delete(struct target_t *target);

int64_t target_mtime(struct target_t *target);

void target_conn(struct target_t *target, struct rule_t *rule);

bool target_equal(const struct target_t *lhs, const struct target_t *rhs);

/*
 * target iterator declarations
 */
struct target_iter_t target_iter(struct target_list_t *list);
struct target_t *target_next(struct target_iter_t *iter);

/*
 * target list declarations
 */
struct target_list_t *target_list_new(void);
void target_list_delete(struct target_list_t *list);

uint32_t target_list_len(struct target_list_t *list);
bool target_list_contains(struct target_list_t *list, struct target_t *target);
void target_list_add(struct target_list_t *list, struct target_t *target);
struct target_t *target_list_find(struct target_list_t *list, bool spec, const char *path);


/**
 * Target map structure.
 *   @ent: The head entry.
 */
struct map_t {
	struct ent_t *ent;
};

/**
 * Entry structure.
 *   @target: The target.
 *   @next: The next entry.
 */
struct ent_t {
	struct target_t *target;
	struct ent_t *next;
};

/*
 * map declarations
 */
struct map_t *map_new(void);
void map_delete(struct map_t *map);

struct target_t *map_get(struct map_t *map, bool spec, const char *path);
void map_add(struct map_t *map, struct target_t *target);


/**
 * Sequence structure.
 *   @head, tail: The head and tail commands.
 */
struct seq_t {
	struct cmd_t *head, **tail;
};

/**
 * Command structure.
 *   @pipe: The command pipe.
 *   @in, out: The input and output files.
 *   @append: Append mode.
 *   @next: The next value.
 */
struct cmd_t {
	struct rt_pipe_t *pipe;
	char *in, *out;
	bool append;

	struct cmd_t *next;
};

/*
 * sequence declarations
 */
struct seq_t *seq_new(void);
void seq_delete(struct seq_t *seq);

void seq_add(struct seq_t *seq, struct rt_pipe_t *pipe, char *in, char *out, bool append);


/**
 * Pipe structure.
 *   @cmd: The command.
 *   @next: The next pipe.
 */
struct rt_pipe_t {
	struct val_t *cmd;
	struct rt_pipe_t *next;
};

/*
 * pipe declarations
 */
struct rt_pipe_t *rt_pipe_new(struct val_t *cmd);
void rt_pipe_clear(struct rt_pipe_t *pipe);



/************************/

/**
 * Graph node structure.
 *   @path: The file path.
 *   @hash: The hash for quick comparisons.
 *   @flags: The flags.
 *   @deps: The number of dependencies.
 *   @edge: The set of forward edges.
 */
struct node_t {
	char *path;
	uint64_t hash;
	uint32_t flags;

	uint32_t deps;
	struct set_t *edge;
};

/*
 * node flags
 */
#define NODE_SPEC (1 << 0)

/*
 * node declarations
 */
struct node_t *node_new(bool spec, char *path);
void node_delete(struct node_t *node);


/**
 * Location structure.
 *   @path: The path.
 *   @lin, col: The line and column information.
 */
struct loc_t {
	const char *path;
	uint32_t lin, col;
};


/*
 * command-line declarations
 */
extern char *cli_app;

void cli_proc(char **args);
void cli_err(const char *fmt, ...) __attribute__((noreturn));


/*
 * location declarations
 */
struct loc_t loc_off(struct loc_t loc, uint32_t off);
void loc_err(struct loc_t loc, const char *fmt, ...) __attribute__((noreturn));


/**
 * Value structure.
 *   @spec: Special flag.
 *   @str: The string.
 *   @next: The next value.
 */
struct val_t {
	bool spec;
	char *str;

	struct val_t *next;
};

/*
 * value declarations
 */
struct val_t *val_new(bool spec, char *str);
struct val_t *val_dup(const struct val_t *val);
void val_delete(struct val_t *val);
void val_clear(struct val_t *val);

char *val_id(struct val_t *val, struct loc_t loc);
char *val_str(struct val_t *val, struct loc_t loc);
uint32_t val_len(struct val_t *val);


/**
 * Binding structure.
 *   @id: The identifier.
 *   @tag: The tag.
 *   @data: The data.
 *   @loc: The location.
 *   @next: The next binding.
 */
enum bind_e { val_v, func_v, ns_v };
union bind_u { struct val_t *val; func_t *func; };
struct bind_t {
	char *id;

	enum bind_e tag;
	union bind_u data;

	struct loc_t loc;

	struct bind_t *next;
};

/*
 * binding declarations
 */
struct bind_t *bind_new(char *id, enum bind_e tag, union bind_u data, struct loc_t loc);
void bind_delete(struct bind_t *bind);
void bind_erase(struct bind_t *bind);

void bind_set(struct bind_t **dst, struct bind_t *src);

struct bind_t *bind_val(char *id, struct val_t *val);
struct bind_t *bind_func(char *id, func_t *func);

/*
 * string converstion declarations
 */
char *get_str(const char **str, struct loc_t loc);
void get_var(const char **str, char *var, struct loc_t loc);

/*
 * string declarations
 */
void str_trim(const char **str);
char *str_fmt(const char *pat, ...);
void str_set(char **dst, char *src);
void str_final(char **str, const char *dir);


/**
 * String structure.
 *   @buf: The character buffer.
 *   @len, max; The current and maximum lengths.
 */
struct buf_t {
	char *str;
	uint32_t len, max;
};

/*
 * string declarations
 */
struct buf_t buf_new(uint32_t init);
void buf_delete(struct buf_t *buf);
char *buf_done(struct buf_t *buf);

void buf_ch(struct buf_t *buf, char ch);
void buf_mem(struct buf_t *buf, const char *mem, uint32_t len);
void buf_str(struct buf_t *buf, const char *str);

/*
 * character declarations
 */
bool ch_space(int ch);
bool ch_alpha(int ch);
bool ch_num(int ch);
bool ch_alnum(int ch);
bool ch_var(int ch);
bool ch_str(int ch);
bool ch_id(int ch);


/**
 * Namespace structure.
 *   @id: The name or null.
 *   @up, next: The parent and sibling namespaces
 *   @bindings: The list of bindings.
 */
struct ns_t {
	char *id;
	struct ns_t *up, *next;

	struct bind_t *bind;
};

/*
 * namespace declarations
 */
struct ns_t *ns_new(char *id, struct ns_t *up);
void ns_delete(struct ns_t *ns);

struct bind_t *ns_get(struct ns_t *ns, const char *id);

void ns_add(struct ns_t *ns, struct bind_t *bind);

struct bind_t *ns_find(struct ns_t *ns, const char *id);
struct bind_t **ns_lookup(struct ns_t *ns, const char *id);


/**
 * Block structure.
 *   @stmt: The list of statements.
 */
struct block_t {
	struct stmt_t *stmt;
};

/*
 * block declarations
 */
struct block_t *block_new(void);
void block_delete(struct block_t *block);

/**
 * Syntatic rule structure.
 *   @gen, dep: The generator and dependency values.
 *   @cmd: The list of commands.
 *   @in, out: The input and output redirect.
 *   @loc: The location.
 */
struct syn_t {
	struct imm_t *gen, *dep;
	struct list_t *cmd;

	struct loc_t loc;
};

/*
 * syntax rule declarations
 */
struct syn_t *syn_new(struct imm_t *gen, struct imm_t *dep, struct loc_t loc);
void syn_delete(struct syn_t *syn);

void syn_add(struct syn_t *syn, struct imm_t *cmd);


/**
 * Processing statement structure.
 *   @pipe: The list of piped commands.
 *   @in, out: The redirect input and output.
 *   @append: The append flag.
 */
struct ast_cmd_t {
	struct ast_pipe_t *pipe;
	struct raw_t *in, *out;
	bool append;
};

/*
 * processing statement declarations
 */
struct ast_cmd_t *ast_cmd_new(struct ast_pipe_t *pipe);
void ast_cmd_delete(struct ast_cmd_t *cmd);


/**
 * Pipe statement.
 *   @imm: The immediate value.
 *   @next: The next pipe.
 */
struct ast_pipe_t {
	struct imm_t *imm;
	struct ast_pipe_t *next;
};

/*
 * pipe declarations
 */
struct ast_pipe_t *ast_pipe_new(struct imm_t *imm);
void ast_pipe_clear(struct ast_pipe_t *bar);


/**
 * Directory structure.
 *   @def: Default flag.
 *   @raw: The raw string.
 *   @block: The optional blcok.
 */
struct dir_t {
	bool def;
	struct raw_t *raw;
	struct block_t *block;
};

/*
 * directory declarations
 */
struct dir_t *dir_new(bool def, struct raw_t *raw, struct block_t *block);
void dir_delete(struct dir_t *dir);


/**
 * Conditional structure.
 */
struct cond_t {
};


/**
 * Loop structure.
 *   @id: Variable binding identifier.
 *   @imm: The immediate value.
 *   @body: The body statement.
 *   @loc: The location.
 */
struct loop_t {
	char *id;
	struct imm_t *imm;
	struct stmt_t *body;

	struct loc_t loc;
};

struct loop_t *loop_new(char *id, struct imm_t *imm, struct stmt_t *body, struct loc_t loc);
void loop_delete(struct loop_t *loop);


/**
 * Statement structure.
 *   @tag: The tag.
 *   @data: The data.
 *   @loc: The location.
 *   @next: The next statement.
 */
enum stmt_e { assign_v, syn_v, dir_v, loop_v, print_v, block_v };
union stmt_u { struct assign_t *assign; struct syn_t *syn; struct dir_t *dir; struct cond_t *conf; struct loop_t *loop; struct print_t *print; struct block_t *block; };
struct stmt_t {
	enum stmt_e tag;
	union stmt_u data;
	struct loc_t loc;

	struct stmt_t *next;
};

/*
 * statement declarations
 */
struct stmt_t *stmt_new(enum stmt_e tag, union stmt_u data, struct loc_t loc);
void stmt_delete(struct stmt_t *stmt);
void stmt_clear(struct stmt_t *stmt);


/**
 * Printing statement.
 *   @imm: The immediate value.
 */
struct print_t {
	struct imm_t *imm;
};

/*
 * print declarations
 */
struct print_t *print_new(struct imm_t *imm);
void print_delete(struct print_t *print);


/**
 * Assignment structure.
 *   @id: The identifier.
 *   @val: The value.
 */
struct assign_t {
	struct raw_t *id;
	struct imm_t *val;
};

/*
 * assignment declarations
 */
struct assign_t *assign_new(struct raw_t *id, struct imm_t *val);
void assign_delete(struct assign_t *assign);


/**
 * String list.
 *   @raw: The raw strings list.
 */
struct imm_t {
	struct raw_t *raw;
};

/*
 * immediate value declaration
 */
struct imm_t *imm_new(void);
void imm_delete(struct imm_t *imm);

uint32_t imm_len(struct imm_t *imm);


/**
 * Raw string structure.
 *   @spec, var: Special and variable flags.
 *   @str: The string.
 *   @loc: The location.
 *   @next: The next raw string.
 */
struct raw_t {
	bool spec, var;

	char *str;
	struct loc_t loc;

	struct raw_t *next;
};

/*
 * raw value declarations
 */
struct raw_t *raw_new(bool spec, bool var, char *str, struct loc_t loc);
struct raw_t *raw_dup(const struct raw_t *raw);
void raw_delete(struct raw_t *raw);
void raw_clear(struct raw_t *raw);


/**
 * Environment structure.
 *   @map: The variable mapping.
 *   @up: The parent environment.
 */
struct env_t {
	struct map0_t *map;

	struct env_t *up;
};

/*
 * evaluation declarations
 */
void eval_top(struct block_t *block, struct ctx_t *ctx);
void eval_block(struct block_t *block, struct ctx_t *ctx, struct env_t *env);
void eval_stmt(struct stmt_t *stmt, struct ctx_t *ctx, struct env_t *env);
struct val_t *eval_imm(struct imm_t *imm, struct ctx_t *ctx, struct env_t *env);
struct val_t *eval_raw(struct raw_t *raw, struct ctx_t *ctx, struct env_t *env);
struct val_t *eval_var(const char **str, struct loc_t loc, struct ctx_t *ctx, struct env_t *env);

/*
 * environment declarations
 */
struct env_t env_new(struct env_t *up);
void env_delete(struct env_t env);

struct bind_t *env_get(struct env_t *env, const char *id);
void env_put(struct env_t *env, struct bind_t *bind);



/**
 * Job structure.
 *   @pid: The pid.
 *   @rule: The rule.
 *   @cmd: The current command.
 */
struct job_t {
	int pid;
	struct rule_t *rule;
	struct cmd_t *cmd;
};

/**
 * Job control structure.
 *   @queue: The rule queue.
 *   @job: The job array.
 *   @cnt: The number of jobs.
 */
struct ctrl_t {
	struct queue_t *queue;

	struct job_t *job;
	uint32_t cnt;
};

/*
 * job control declarations
 */
struct ctrl_t *ctrl_new(struct queue_t *queue, uint32_t n);
void ctrl_delete(struct ctrl_t *ctrl);

void ctrl_add(struct ctrl_t *ctrl, struct rule_t *rule);
bool ctrl_avail(struct ctrl_t *ctrl);
bool ctrl_busy(struct ctrl_t *ctrl);
void ctrl_wait(struct ctrl_t *ctrl);
void ctrl_done(struct ctrl_t *ctrl, struct rule_t *rule);


/*
 * builtin function declarations
 */
struct val_t *fn_sub(struct val_t **args, uint32_t cnt, struct loc_t loc);
struct val_t *fn_pat(struct val_t **args, uint32_t cnt, struct loc_t loc);


/**
 * Map structure.
 *   @ent: The entity list.
 *   @cmp: Comparison function.
 *   @del: Delete function.
 */
struct map1_t {
	struct ent1_t *ent;
	void (*cmp)(const void *, const void *);
	void (*del)(void *);
};

/**
 * Entity structure.
 *   @id: The identifier.
 *   @ref: The reference.
 *   @next: The next entity.
 */
struct ent1_t {
	const char *id;
	void *ref;

	struct ent1_t *next;
};


/**
 * Entry structure.
 *   @key: The key.
 *   @val: The value.
 *   @next: The next entry.
 */
struct entry0_t {
	const void *key;
	void *val;

	struct entry0_t *next;
};

/**
 * Map structure.
 *   @cmp: The comparison function.
 *   @del: The deletion function.
 */
struct map0_t {
	cmp_f cmp;
	del_f del;

	struct entry0_t *entry;
};

/*
 * map declarations
 */
struct map0_t *map0_new(cmp_f cmp, del_f del);
void map0_delete(struct map0_t *map);

void map0_add(struct map0_t *map, const void *key, void *val);
void *map0_get(struct map0_t *map, const void *key);
void *map_rem(struct map0_t *map, const void *key);


/**
 * Main entry point.
 *   @argc: The number of arguments.
 *   @argv: The argument array.
 *   &returns: Exit status.
 */
int main(int argc, char **argv)
{
	cli_app = "hammer";

	os_init();
	cli_proc(argv + 1);
	
	return 0;
}


/**
 * Create a 64-bit hash of a string.
 *   @hash: The starting hash.
 *   @str: The string.
 *   &returns: The hash.
 */
uint64_t hash64(uint64_t hash, const char *str)
{
	while(*str != '\0') {
		hash += (uint64_t)*str++ ^ 0x687ebde0fd11d4ae;
		hash *= 0xe278b9fdc665d444;
		hash ^= (hash >> 53);
		hash *= 0x19ba967de10aacc0;
		hash ^= (hash >> 48);
		hash *= 0x1b2b216668c9e998;
		hash ^= (hash >> 43);
		hash *= 0x73c3d43eb467f8df;
		hash ^= (hash >> 37);
	}

	return hash;
}


/**
 * Compute an offset from a location.
 *   @loc: The location.
 *   @off: The offset.
 *   &returns: The new location.
 */
struct loc_t loc_off(struct loc_t loc, uint32_t off)
{
	return (struct loc_t){ loc.path, loc.lin, loc.col + off };
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
	syn->cmd = list_new((del_f)ast_cmd_delete);

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
 * Create a new processing statement.
 *   @pipe: Consumed. The pipe list.
 *   &returns: The processing statement.
 */
struct ast_cmd_t *ast_cmd_new(struct ast_pipe_t *pipe)
{
	struct ast_cmd_t *proc;

	proc = malloc(sizeof(struct ast_cmd_t));
	proc->pipe = pipe;
	proc->in = proc->out = NULL;
	proc->append = false;

	return proc;
}

/**
 * Delete a processing statement.
 *   @proc: The processing statement.
 */
void ast_cmd_delete(struct ast_cmd_t *cmd)
{
	if(cmd->in != NULL)
		raw_delete(cmd->in);

	if(cmd->out != NULL)
		raw_delete(cmd->out);

	ast_pipe_clear(cmd->pipe);
	free(cmd);
}


/**
 * Create a new pipe. 
 *  @imm: The immediate value.
 *  &returns: The pipe.
 */
struct ast_pipe_t *ast_pipe_new(struct imm_t *imm)
{
	struct ast_pipe_t *pipe;

	pipe = malloc(sizeof(struct ast_pipe_t));
	pipe->imm = imm;
	pipe->next = NULL;

	return pipe;
}

/**
 * Clear a pipe chain.
 *   @pipe: The pipe.
 */
void ast_pipe_clear(struct ast_pipe_t *pipe)
{
	struct ast_pipe_t *tmp;

	while(pipe != NULL) {
		pipe = (tmp = pipe)->next;
		imm_delete(tmp->imm);
		free(tmp);
	}
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
 * Create a loop.
 *   @id: Consumed. The identifier.
 *   @imm: Consumed. The immediate value.
 *   @body: Consumed. The body statement.
 *   @loc: The location.
 *   &returns: The loop.
 */
struct loop_t *loop_new(char *id, struct imm_t *imm, struct stmt_t *body, struct loc_t loc)
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
	case loop_v: loop_delete(stmt->data.loop); break;
	case print_v: print_delete(stmt->data.print); break;
	case block_v: block_delete(stmt->data.block); break;
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

struct block_t *rd_top(struct rd_t *rd);
struct stmt_t *rd_stmt(struct rd_t *rd);
struct imm_t *rd_imm(struct rd_t *rd);
struct raw_t *rd_raw(struct rd_t *rd);


/**
 * Load a hammer file from the top.
 *   @path: The path.
 *   &returns: The block.
 */
struct block_t *ham_load(const char *path)
{
	struct rd_t rd;
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
#define TOK_STR   0x1000
#define TOK_VAR   0x1001
#define TOK_SPEC  0x1002
#define TOK_DIR   0x2000
#define TOK_FOR   0x2001
#define TOK_IF    0x2002
#define TOK_ELIF  0x2003
#define TOK_ELSE  0x2004
#define TOK_PRINT 0x2005
#define TOK_DEF   0x2006
#define TOK_SHR   0x3000
#define TOK_SHL   0x3001
#define TOK_EOF   0x7FFF

struct sym_t syms[] = {
	{ TOK_SHR, ">>" },
	{ TOK_SHL, "<<" },
	{ '{',     "{" },
	{ '}',     "}" },
	{ ':',     ":" },
	{ ';',     ";" },
	{ '=',     "=" },
	{ '<',     "<" },
	{ '>',     ">" },
	{ '|',     "|" },
	{ 0,       NULL }
};

struct sym_t keys[] = {
	{ TOK_DIR,   "dir"     },
	{ TOK_FOR,   "for"     },
	{ TOK_IF,    "if"      },
	{ TOK_ELIF,  "elif"    },
	{ TOK_ELSE,  "else"    },
	{ TOK_PRINT, "print"   },
	{ TOK_DEF  , "default" },
	{ 0,         NULL      }
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

	if(strchr("tn'\" $", rd_ch(rd)) == NULL)
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
bool rd_var(struct rd_t *rd) {
	if(rd->ch != '$')
		return false;

	rd_buf(rd);
	if(rd->ch == '{') {
		rd_buf(rd);

		for(;;) {
			if(rd->ch == '\\') {
				char ch;

				switch(rd_ch(rd)) {
				case 't': ch = '\t'; break;
				case 'n': ch = '\n'; break;
				case '\'': ch = '\''; break;
				case ' ': ch = ' '; break;
				case '"': ch = '"'; break;
				case '$': ch = '$'; break;
				default: loc_err(rd->tloc, "Invalid escape character '\\%c'.", rd->ch);
				}

				rd_push(rd, ch);
				rd_ch(rd);
			}
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
	else if(strchr("@^<~", rd->ch) != NULL)
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
		if(rd->ch == '\\') {
			char ch;

			switch(rd_ch(rd)) {
			case 't': ch = '\t'; break;
			case 'n': ch = '\n'; break;
			case '\'': ch = '\''; break;
			case ' ': ch = ' '; break;
			case '"': ch = '"'; break;
			case '$': ch = '$'; break;
			default: loc_err(rd->loc, "Invalid escape character '\\%c'.", rd->ch);
			}

			rd_push(rd, ch);
			rd_ch(rd);
		}
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
struct block_t *rd_block(struct rd_t *rd)
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
struct block_t *rd_top(struct rd_t *rd)
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
struct stmt_t *rd_stmt(struct rd_t *rd)
{
	struct loc_t loc;
	enum stmt_e tag;
	union stmt_u data;

	loc = rd->loc;
	if((rd->tok == TOK_STR) || (rd->tok == TOK_SPEC) || (rd->tok == TOK_VAR)) {
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
	else if(rd->tok == TOK_FOR) {
		char *id;
		struct imm_t *imm;
		struct stmt_t *body;
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


/**
 * Create a new binding.
 *   @id: Consumed. The identifier.
 *   @tag: The tag.
 *   @data: Consumed. The data.
 *   @loc: The location.
 *   &returns: The binding.
 */
struct bind_t *bind_new(char *id, enum bind_e tag, union bind_u data, struct loc_t loc)
{
	struct bind_t *bind;

	bind = malloc(sizeof(struct bind_t));
	*bind = (struct bind_t){ id, tag, data, loc, NULL };

	return bind;
}

/**
 * Delete a binding.
 *   @bind: The binding.
 */
void bind_delete(struct bind_t *bind)
{
	switch(bind->tag) {
	case val_v: val_clear(bind->data.val); break;
	case func_v: break;
	case ns_v: cli_err("FIXME stub");
	}

	free(bind->id);
	free(bind);
}

/**
 * Delete a binding if non-null.
 *   @bind: The bind.
 */
void bind_erase(struct bind_t *bind)
{
	if(bind != NULL)
		bind_delete(bind);
}


/**
 * Set a new binding, replacing the old value.
 *   @dst: The destination.
 *   @src: The source.
 */
void bind_set(struct bind_t **dst, struct bind_t *src)
{
	if(*dst != NULL)
		bind_delete(*dst);

	*dst = src;
}


/**
 * Create a value binding.
 *   @id: The identifier.
 *   @val: The value.
 *   @loc: The location.
 *   &returns: The binding.
 */
struct bind_t *bind_val(char *id, struct val_t *val)
{
	return bind_new(id, val_v, (union bind_u){ .val = val }, (struct loc_t){ });
}

/**
 * Create a function binding.
 *   @id: The identifier.
 *   @rule: The rule.
 *   &returns: The binding.
 */
struct bind_t *bind_func(char *id, func_t *func)
{
	return bind_new(id, func_v, (union bind_u){ .func = func }, (struct loc_t){ });
}


/**
 * Create a value.
 *   @spec: Special value flag.
 *   @str: Consumed. The string.
 *   &returns: The value.
 */
struct val_t *val_new(bool spec, char *str)
{
	struct val_t *val;

	val = malloc(sizeof(struct val_t));
	val->spec = spec;
	val->str = str;
	val->next = NULL;

	return val;
}

/**
 * Duplicate a value.
 *   @val: The value.
 *   &returns: The duplicated value.
 */
struct val_t *val_dup(const struct val_t *val)
{
	struct val_t *ret, **iter;

	iter = &ret;
	while(val != NULL) {
		*iter = malloc(sizeof(struct val_t));
		(*iter)->spec = val->spec;
		(*iter)->str = strdup(val->str);
		iter = &(*iter)->next;
		val = val->next;
	}

	*iter = NULL;
	return ret;
}

/**
 * Delete a value.
 *   @val: The value.
 */
void val_delete(struct val_t *val)
{
	free(val->str);
	free(val);
}

/**
 * Clear a list of values.
 *   @val: The value list.
 */
void val_clear(struct val_t *val)
{
	struct val_t *tmp;

	while(val != NULL) {
		val = (tmp = val)->next;
		val_delete(tmp);
	}
}


/**
 * Unwrap an identifier from a value.
 *   @val: Consumed. The value.
 *   @loc: The location.
 *   &returns: The allocated identifier.
 */
char *val_id(struct val_t *val, struct loc_t loc)
{
	char *id;

	if((val == NULL) || (val_len(val) >= 2))
		loc_err(loc, "Invalid variable name.");

	id = val->str;
	free(val);

	return id;
}

/**
 * Unwrap an single string from a value.
 *   @val: Consumed. The value.
 *   @loc: The location.
 *   &returns: The allocated identifier.
 */
char *val_str(struct val_t *val, struct loc_t loc)
{
	char *id;

	if((val == NULL) || (val_len(val) >= 2))
		loc_err(loc, "Must be a single string.");

	id = val->str;
	free(val);

	return id;
}

/**
 * Retrieve the value length.
 *   @val: The value.
 *   &returns: The length.
 */
uint32_t val_len(struct val_t *val)
{
	uint32_t n = 0;

	while(val != NULL) {
		n++;
		val = val->next;
	}

	return n;
}

/*
 * variables
 */
char *cli_app = NULL;


/**
 * Initialize an array or string.
 *   @arr: Out. The array.
 *   @cnt: Out. The count.
 */
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
	opt.jobs = -1;
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
					bool end = false;

					switch(args[i][k]) {
					case 'B': opt.force = true; break;

					case 'd':
						if(opt.dir != NULL)
							cli_err("Directory already given.");
						else if(args[i][k + 1] == '\0') {
							if((opt.dir = args[++i]) == NULL)
								cli_err("Missing directory (-d).");
						}
						else
							opt.dir = args[i] + k + 1;

						end = true;
						break;

					case 'j': {
						char *endptr;
						const char *str;
						unsigned long val;

						if(opt.jobs >= 0)
							cli_err("Jobs (-j) already given.");
						else if(args[i][k + 1] == '\0') {
							if((str = args[++i]) == NULL)
								cli_err("Missing job count (-j).");
						}
						else
							str = args[i] + k + 1;

						errno = 0;
						val = strtol(str, &endptr, 0);
						if((errno != 0) || (*endptr != '\0') || (val == 0))
							cli_err("Invalid job count (-j).");

						opt.jobs = (val > 1024) ? 1024 : val;

						k = strlen(args[i]);
						end = true;
					} break;

					default: cli_err("Unknown option '-%c'.", args[i][k]); 
					}

					if(end)
						break;

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


/**
 * Create a command sequence.
 *   &returns: The sequence.
 */
struct seq_t *seq_new(void)
{
	struct seq_t *seq;

	seq = malloc(sizeof(struct seq_t));
	*seq = (struct seq_t){ NULL, &seq->head };

	return seq;
}

/**
 * Delete a sequence.
 *   @seq: The sequence.
 */
void seq_delete(struct seq_t *seq)
{
	struct cmd_t *cmd, *tmp;

	cmd = seq->head;
	while(cmd != NULL) {
		cmd = (tmp = cmd)->next;
		rt_pipe_clear(tmp->pipe);
		free(tmp);
	}

	free(seq);
}


/**
 * Add a command to a sequence.
 *   @seq: The sequence.
 *   @pipe: The pipe sequence.
 *   @in: Optional. The input path.
 *   @out: Optional. The output path.
 *   @append: The append flag.
 */
void seq_add(struct seq_t *seq, struct rt_pipe_t *pipe, char *in, char *out, bool append)
{
	struct cmd_t *cmd;

	cmd = malloc(sizeof(struct cmd_t));
	*cmd = (struct cmd_t){ pipe, in, out, append, NULL };

	*seq->tail = cmd;
	seq->tail = &cmd->next;
}


/**
 * Create a pipe.
 *   @cmd: Consumed. The pipe.
 *   &returns: The pipe.
 */
struct rt_pipe_t *rt_pipe_new(struct val_t *cmd)
{
	struct rt_pipe_t *pipe;

	pipe = malloc(sizeof(struct rt_pipe_t));
	pipe->cmd = cmd;
	pipe->next = NULL;

	return pipe;
}

/**
 * Clear a list of pipes.
 *   @pipe: The pipe list.
 */
void rt_pipe_clear(struct rt_pipe_t *pipe)
{
	struct rt_pipe_t *tmp;

	while(pipe != NULL) {
		pipe = (tmp = pipe)->next;
		val_delete(tmp->cmd);
		free(tmp);
	}
}


/**
 * Create a new context.
 *   @opt: The options structure.
 *   &returns: The context.
 */
struct ctx_t *ctx_new(const struct opt_t *opt)
{
	struct ctx_t *ctx;

	ctx = malloc(sizeof(struct ctx_t));
	ctx->opt = opt;
	ctx->map = map_new();
	ctx->rules = rule_list_new();
	ctx->dir = bind_val(strdup("~"), val_new(false, strdup(opt->dir ? opt->dir : "")));
	ctx->gens = ctx->deps = NULL;
	ctx->gen = ctx->dep = NULL;

	return ctx;
}

/**
 * Delete the context.
 *   @ctx: The context.
 */
void ctx_delete(struct ctx_t *ctx)
{
	bind_delete(ctx->dir);
	map_delete(ctx->map);
	rule_list_delete(ctx->rules);
	free(ctx);
}


/**
 * Run all outdated rules on the context.
 *   @ctx: The context.
 *   @builds: The set of target to build.
 */
void ctx_run(struct ctx_t *ctx, const char **builds)
{
	struct ctrl_t *ctrl;
	struct rule_t *rule;
	struct rule_iter_t irule;
	struct queue_t *queue;

	queue = queue_new();
	ctrl = ctrl_new(queue, 4);

	irule = rule_iter(ctx->rules);
	while((rule = rule_next(&irule)) != NULL) {
		uint32_t i;
		struct target_t *target;
		struct target_iter_t iter;

		iter = target_iter(rule->gens);
		while((target = target_next(&iter)) != NULL) {
			for(i = 0; builds[i] != NULL; i++) {
				if(strcmp(target->path, builds[i]) == 0)
					queue_recur(queue, rule);
			}

			if((target->flags & (FLAG_BUILD | FLAG_SPEC)) == 0)
				target->flags |= FLAG_BUILD;
		}
	}

	for(;;) {
		while(!ctrl_avail(ctrl))
			ctrl_wait(ctrl);

		rule = queue_rem(queue);
		if(rule == NULL) {
			if(!ctrl_busy(ctrl))
				break;

			ctrl_wait(ctrl);
			continue;
		}

		struct target_t *target;
		struct target_iter_t iter;
		int64_t min = INT64_MAX - 1, max = INT64_MIN + 1;

		iter = target_iter(rule->gens);
		while((target = target_next(&iter)) != NULL) {
			if(target->flags & FLAG_SPEC)
				min = INT64_MIN, max = INT64_MAX;

			if(target_mtime(target) < min)
				min = target_mtime(target);
		}

		iter = target_iter(rule->deps);
		while((target = target_next(&iter)) != NULL) {
			if(target->flags & FLAG_SPEC)
				continue;

			if(target_mtime(target) > max)
				max = target_mtime(target);
		}

		iter = target_iter(rule->gens);
		if((max > min) || ctx->opt->force) {
			iter = target_iter(rule->gens);
			while((target = target_next(&iter)) != NULL) {
				char *path, *iter;

				if(target->flags & FLAG_SPEC)
					continue;

				//FIXME parent directory option
				path = strdup(target->path);
				iter = path;
				while((iter = strchr(iter, '/')) != NULL) {
					*iter = '\0';
					os_mkdir(path);
					*iter = '/';
					iter++;
				}

				free(path);
			}

			ctrl_add(ctrl, rule);
		}
		else
			ctrl_done(ctrl, rule);
	}

	while(ctrl_busy(ctrl))
		ctrl_wait(ctrl);

	queue_delete(queue);
	ctrl_delete(ctrl);
}


/**
 * Retrieve a target, creating it if required.
 *   @ctx: The context.
 *   @spec: The special flag.
 *   @path: The path.
 *   &returns: The target.
 */
struct target_t *ctx_target(struct ctx_t *ctx, bool spec, const char *path)
{
	struct target_t *target;

	target = map_get(ctx->map, spec, path);
	if(target == NULL) {
		target = target_new(spec, strdup(path));
		map_add(ctx->map, target);
	}

	return target;
}

/**
 * Retrieve a rule, creating it if required.
 *   @ctx: The context.
 *   @id: Optional. The rule identifier.
 *   @gens: Consumed. The set of generated targets.
 *   @deps: Consumed. The set of dependency targets.
 *   @seq: Consumed. The command sequence.
 */
struct rule_t *ctx_rule(struct ctx_t *ctx, const char *id, struct target_list_t *gens, struct target_list_t *deps, struct seq_t *seq)
{
	struct rule_t *rule;

	if(id != NULL) {
		fatal("FIXME stub rule w/ id");
	}
	else {
		struct target_t *target;
		struct target_iter_t iter;

		rule = rule_new(id ? strdup(id) : NULL, gens, deps, seq);
		rule_list_add(ctx->rules, rule);

		iter = target_iter(gens);
		while((target = target_next(&iter)) != NULL) {
			if(target->rule != NULL)
				fatal("FIXME target already had rule, better error");

			target->rule = rule;
		}

		iter = target_iter(deps);
		while((target = target_next(&iter)) != NULL)
			target_conn(target, rule);
	}

	return NULL;
}

struct val_t *func_sub(struct ctx_t *ctx, struct val_t **args, uint32_t cnt)
{
	return NULL;
}


/**
 * Evaluate from the top.
 *   @block: The block.
 *   @ctx: The context.
 */
void eval_top(struct block_t *block, struct ctx_t *ctx)
{
	struct env_t env;
	struct stmt_t *stmt;

	env = env_new(NULL);
	env_put(&env, bind_func(strdup(".sub"), fn_sub));
	env_put(&env, bind_func(strdup(".pat"), fn_pat));

	for(stmt = block->stmt; stmt != NULL; stmt = stmt->next)
		eval_stmt(stmt, ctx, &env);

	env_delete(env);
}

/**
 * Evaluate a block.
 *   @block: The block.
 *   @ctx: The context.
 */
void eval_block(struct block_t *block, struct ctx_t *ctx, struct env_t *env)
{
	struct stmt_t *stmt;

	for(stmt = block->stmt; stmt != NULL; stmt = stmt->next)
		eval_stmt(stmt, ctx, env);
}

/**
 * Evaluate a statement.
 *   @stmt: The statement.
 *   @ctx: The context.
 *   @env: The environment.
 */
void eval_stmt(struct stmt_t *stmt, struct ctx_t *ctx, struct env_t *env)
{
	switch(stmt->tag) {
	case assign_v: {
		char *id;
		struct val_t *val;
		struct assign_t *assign = stmt->data.assign;

		id = val_id(eval_raw(assign->id, ctx, env), stmt->loc);
		val = eval_imm(assign->val, ctx, env);

		env_put(env, bind_val(id, val));
	} break;

	case syn_v: {
		struct seq_t *seq;
		struct syn_t *syn = stmt->data.syn;
		struct target_list_t *gens, *deps;
		struct val_t *gen, *dep, *iter;
		struct link_t *link;
		struct ast_cmd_t *proc;

		seq = seq_new();
		gens = target_list_new();
		deps = target_list_new();

		gen = eval_imm(syn->gen, ctx, env);
		for(iter = gen; iter != NULL; iter = iter->next)
			target_list_add(gens, ctx_target(ctx, iter->spec, iter->str));

		dep = eval_imm(syn->dep, ctx, env);
		for(iter = dep; iter != NULL; iter = iter->next)
			target_list_add(deps, ctx_target(ctx, iter->spec, iter->str));

		ctx->gen = gen;
		ctx->dep = dep;
		for(link = syn->cmd->head; link != NULL; link = link->next) {
			char *out, *in;

			proc = link->val;
			in = proc->in ? val_str(eval_raw(proc->in, ctx, env), syn->loc) : NULL;
			out = proc->out ? val_str(eval_raw(proc->out, ctx, env), syn->loc) : NULL;

			struct ast_pipe_t *iter;
			struct rt_pipe_t *pipe = NULL, **ipipe = &pipe;

			for(iter = proc->pipe; iter != NULL; iter = iter->next) {
				*ipipe = rt_pipe_new(eval_imm(iter->imm, ctx, env));
				ipipe = &(*ipipe)->next;
			}

			seq_add(seq, pipe, in, out, proc->append);
		}

		ctx->gen = ctx->dep = NULL;
		val_clear(gen);
		val_clear(dep);
		ctx_rule(ctx, NULL, gens, deps, seq);
	} break;

	case dir_v: {
		char *str;
		bool cont;
		struct dir_t *dir = stmt->data.dir;

		str = val_str(eval_raw(dir->raw, ctx, env), dir->raw->loc);
		if(ctx->dir == NULL) {
			cont = dir->def;
			if(cont)
				bind_set(&ctx->dir, bind_val(strdup("~"), val_new(false, strdup(str))));
		}
		else
			cont = (strcmp(str, ctx->dir->data.val->str) == 0);

		free(str);

		if(cont && (dir->block != NULL))
			eval_block(dir->block, ctx, env);
	} break;

	case loop_v: {
		struct env_t nest;
		struct val_t *val, *iter;
		struct loop_t *loop = stmt->data.loop;

		val = eval_imm(loop->imm, ctx, env);

		for(iter = val; iter != NULL; iter = iter->next) {
			nest = env_new(env);
			env_put(&nest, bind_val(strdup(loop->id), val_new(iter->spec, strdup(iter->str))));
			eval_stmt(loop->body, ctx, &nest);
			env_delete(nest);
		}

		val_clear(val);
	} break;

	case print_v: {
		struct val_t *val, *iter;

		val = eval_imm(stmt->data.print->imm, ctx, env);

		for(iter = val; iter != NULL; iter = iter->next)
			print("%s%s", iter->str, iter->next ? " " : "");

		val_clear(val);
	} break;

	case block_v: {
		struct env_t nest;

		nest = env_new(env);
		eval_block(stmt->data.block, ctx, &nest);
		env_delete(nest);
	} break;
	}
}

/**
 * Evaluate an immediate value.
 *   @imm: The immediate value.
 *   @ctx: The context.
 *   @env: The environment.
 */
struct val_t *eval_imm(struct imm_t *imm, struct ctx_t *ctx, struct env_t *env)
{
	struct raw_t *raw;
	struct val_t *val = NULL, **iter = &val;

	for(raw = imm->raw; raw != NULL; raw = raw->next) {
		*iter = eval_raw(raw, ctx, env);
		while(*iter != NULL)
			iter = &(*iter)->next;
	}

	return val;
}


/**
 * Expander structure.
 *   @buf: The string buffer.
 *   @orig, str: The origin and active string pointers.
 *   @ctx: The context.
 *   @env: The environment.
 *   @loc: The location.
 */
struct exp_t {
	struct buf_t buf;
	const char *orig, *str;

	struct ctx_t *ctx;
	struct env_t *env;
	struct loc_t loc;
};


/*
 * expansion declarations
 */
struct val_t *exp_get(struct exp_t *exp);

char exp_ch(struct exp_t *exp);
char exp_adv(struct exp_t *exp);
char exp_buf(struct exp_t *exp);
char exp_trim(struct exp_t *exp);

void exp_str(struct exp_t *exp);
void exp_escape(struct exp_t *exp);
void exp_quote1(struct exp_t *exp);
void exp_quote2(struct exp_t *exp);
struct val_t *exp_var(struct exp_t *exp);
struct bind_t *exp_bind(struct exp_t *exp);
void exp_flat(struct exp_t *expr, struct val_t *val);

__attribute__((noreturn)) void exp_err(struct exp_t *exp, const char *fmt, ...);


/**
 * Get an expaneded a value.
 *   @exp: The expander.
 *   &returns: The value.
 */
struct val_t *exp_get(struct exp_t *exp)
{
	struct val_t *val;
	struct buf_t tmp;

	tmp = exp->buf;
	exp->buf = buf_new(32);

	if(*exp->str == '$') {
		val = exp_var(exp);
		if(*exp->str != '\0') {
			exp_flat(exp, val);
			exp_str(exp);
			val = val_new(false, strdup(buf_done(&exp->buf)));
		}
	}
	else if(*exp->str == '.') {
		exp_buf(exp);
		while(ch_var(exp_ch(exp)))
			exp_buf(exp);

		if(*exp->str != '\0') {
			exp_str(exp);
			val = val_new(false, strdup(buf_done(&exp->buf)));
		}
		else
			val = val_new(true, strdup(buf_done(&exp->buf)));
	}
	else {
		exp_str(exp);
		val = val_new(false, strdup(buf_done(&exp->buf)));
	}

	buf_delete(&exp->buf);
	exp->buf = tmp;

	return val;
}


/**
 * Advance the expander a character.
 *   @exp: The expander.
 *   &returns: The next character.
 */
char exp_ch(struct exp_t *exp)
{
	return *exp->str;
}

/**
 * Advance the expander a character.
 *   @exp: The expander.
 *   &returns: The next character.
 */
char exp_adv(struct exp_t *exp)
{
	exp->str++;

	return *exp->str;
}

/**
 * Buffer the current character on an expander.
 *   @exp: The expander.
 *   &returns: The next character.
 */
char exp_buf(struct exp_t *exp)
{
	buf_ch(&exp->buf, *exp->str);

	return exp_adv(exp);
}

/**
 * Trim whitespace from an expander.
 *   @exp: The expander.
 *   &returns: The next character.
 */
char exp_trim(struct exp_t *exp)
{
	const char *str;

	str = exp->str;
	while((*str == ' ') || (*str == '\t') || (*str == '\n'))
		str++;

	exp->str = str;
	return *str;
}


/**
 * Expand a string.
 *   @exp: The expander.
 */
void exp_str(struct exp_t *exp)
{
	char ch;

	for(;;) {
		ch = exp_ch(exp);
		if(ch == '\0')
			break;
		else if(ch == '\\')
			exp_escape(exp);
		else if(ch == '\'')
			exp_quote1(exp);
		else if(ch == '"')
			exp_quote2(exp);
		else if(ch == '$')
			exp_flat(exp, exp_var(exp));
		else if(ch_str(ch))
			exp_buf(exp);
		else
			break;
	}
}

/**
 * Expand an escape sequence.
 *   @exp: The expander.
 */
void exp_escape(struct exp_t *exp)
{
	char ch;

	switch(exp_adv(exp)) {
	case '\\': ch = '\\'; break;
	case '\'': ch = '\''; break;
	case '\"': ch = '\"'; break;
	case 't': ch = '\t'; break;
	case 'n': ch = '\n'; break;
	case '$': ch = '$'; break;
	case ' ': ch = ' '; break;
	default: exp_err(exp, "Invalid escape sequence '\\%c'.", exp_ch(exp));
	}

	buf_ch(&exp->buf, ch);
	exp_adv(exp);
}

/**
 * Expand a single-quoted string.
 *   @exp: The expander.
 */
void exp_quote1(struct exp_t *exp)
{
	char ch;

	exp_adv(exp);
	
	for(;;) {
		ch = exp_ch(exp);
		if(ch == '\'')
			break;
		else if(ch == '\\')
			exp_escape(exp);
		else
			exp_buf(exp);
	}

	exp_adv(exp);
}

/**
 * Expand a double-quoted string.
 *   @exp: The expander.
 */
void exp_quote2(struct exp_t *exp)
{
	char ch;

	exp_adv(exp);
	
	for(;;) {
		ch = exp_ch(exp);
		if(ch == '"')
			break;
		else if(ch == '\\')
			exp_escape(exp);
		else if(ch == '$')
			exp_flat(exp, exp_var(exp));
		else
			exp_buf(exp);
	}

	exp_adv(exp);
}


/**
 * Expand a variable.
 *   @exp: The expander.
 *   &returns: The value.
 */
struct val_t *exp_var(struct exp_t *exp)
{
	char ch;
	struct val_t *val;
	struct bind_t *bind;

	ch = exp_adv(exp);
	if(ch == '{') {
		exp_adv(exp);
		bind = exp_bind(exp);

		if(bind->tag == func_v)
			exp_err(exp, "Function '%s' used as a value.", bind->id);

		if(bind->tag == ns_v) {
			fatal("FIXME stub ns");
		}

		val = val_dup(bind->data.val);
		while((ch = exp_trim(exp)) != '}') {
			uint32_t cnt;
			const char *id;
			struct val_t **args;
			struct buf_t buf;

			if(ch != '.')
				exp_err(exp, "Expected '.' or '}'.");

			buf = buf_new(32);
			buf_ch(&buf, ch);

			exp_adv(exp);
			ch = exp_trim(exp);
			if(!ch_var(ch))
				exp_err(exp, "Expected function name.");

			do
				buf_ch(&buf, ch);
			while(ch_var(ch = exp_adv(exp)));

			id = buf_done(&buf);
			bind = env_get(exp->env, id);
			if(bind == NULL)
				exp_err(exp, "Unknown function '%s'.", id);

			args_init(&args, &cnt);
			args_add(&args, &cnt, val);

			if(exp_trim(exp) != '(')
				exp_err(exp, "Expected '('.");

			exp_adv(exp);
			if(exp_trim(exp) != ')') {
				for(;;) {
					args_add(&args, &cnt, exp_get(exp));
					ch = exp_trim(exp);
					if(ch == ')')
						break;
					else if(ch != ',')
						exp_err(exp, "Expected ',' or '('.");

					exp_adv(exp);
					exp_trim(exp);
				}
			}

			exp_adv(exp);

			if(bind->tag == val_v)
				exp_err(exp, "Variable '%s' used as a function.", id);
			else if(bind->tag == ns_v)
				exp_err(exp, "Namespace '%s' used as a function.", id);

			val = bind->data.func(args, cnt, exp->loc);

			args_delete(args, cnt);
			buf_delete(&buf);
		}

		exp_adv(exp);
	}
	else {
		bind = exp_bind(exp);

		switch(bind->tag) {
		case val_v: val = val_dup(bind->data.val); break;
		case func_v: exp_err(exp, "Cannot use function as a value.");
		case ns_v: exp_err(exp, "Cannot use namspace as a value.");
		default: fatal("Unreachable.");
		}
	}

	return val;
}

/**
 * Flatten a value.
 *   @exp: The expander.
 *   @val: The value.
 */
void exp_flat(struct exp_t *exp, struct val_t *val)
{
	struct val_t *orig = val;

	while(val != NULL) {
		buf_str(&exp->buf, val->str);

		if(val->next != NULL)
			buf_ch(&exp->buf, ' ');

		val = val->next;
	}

	val_clear(orig);
}

/**
 * Retrieve a variable binding.
 *   @exp: The expander.
 *   &returns: The value.
 */
struct bind_t *exp_bind(struct exp_t *exp)
{
	char *id;
	const char *str;
	struct buf_t buf;
	struct bind_t *bind;

	if(*exp->str == '~') {
		exp_adv(exp);
		return exp->ctx->dir;
	}

	str = exp->str;

	buf = buf_new(32);

	do
		buf_ch(&buf, *str++);
	while(ch_var(*str));

	id = buf_done(&buf);
	bind = env_get(exp->env, id);
	if(bind == NULL)
		exp_err(exp, "Unknown variable '%s'.", id);

	exp->str = str;
	free(id);

	return bind;
}


/**
 * Display an error for string expansion.
 *   @exp: The string expansion.
 *   @fmt: The format string.
 *   @...: The arguments.
 */
void exp_err(struct exp_t *exp, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	fprintf(stderr, "%s:%u:%lu: ", exp->loc.path, exp->loc.lin, exp->loc.col + (exp->str - exp->orig));
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);

	exit(1);
}


/**
 * Evaluate a raw value.
 *   @raw: The raw.
 *   @ctx: The context.
 *   @env: The environment.
 */
struct val_t *eval_raw(struct raw_t *raw, struct ctx_t *ctx, struct env_t *env)
{
	struct exp_t exp;

	exp.orig = exp.str = raw->str;
	exp.loc = raw->loc;
	exp.ctx = ctx;
	exp.env = env;

	return exp_get(&exp);
}


/**
 * Initialize arguments.
 *   @args: The arguments reference.
 *   @cnt: The number of arguments reference.
 */
void args_init(struct val_t ***args, uint32_t *cnt)
{
	*args = malloc(0);
	*cnt = 0;
}

/**
 * Add to arguments.
 *   @args: The arguments reference.
 *   @cnt: The number of arguments reference.
 *   @val: Consumed. The value.
 */
void args_add(struct val_t ***args, uint32_t *cnt, struct val_t *val)
{
	*args = realloc(*args, (*cnt + 1) * sizeof(struct val_t));
	(*args)[(*cnt)++] = val;
}

/**
 * Delete arguments.
 *   @args: The arguments.
 *   @cnt: The number of arguments.
 */
void args_delete(struct val_t **args, uint32_t cnt)
{
	uint32_t i;

	for(i = 0; i < cnt; i++)
		val_delete(args[i]);

	free(args);
}


struct val_t *fn_sub(struct val_t **args, uint32_t cnt, struct loc_t loc)
{
	struct buf_t buf;
	struct val_t *val, *ret, **iter;
	const char *str, *find;

	if(cnt != 3)
		loc_err(loc, "Function `.sub` requires 2 arguments.");

	iter = &ret;

	for(val = args[0]; val != NULL; val = val->next) {
		str = val->str;
		buf = buf_new(strlen(val->str) + 1);

		find = strstr(str, args[1]->str);
		while(find != NULL) {
			buf_mem(&buf, str, find - str);
			buf_str(&buf, args[2]->str);

			str = find + strlen(args[1]->str);
			find = strstr(str, args[1]->str);
		}
		buf_str(&buf, str);

		*iter = val_new(val->spec, buf_done(&buf));
		iter = &(*iter)->next;
	}

	*iter = NULL;

	return ret;
}

/**
 * Compute the pre and post lengths of a pattern.
 *   @str: The string.
 *   @pre: Out. The pre-pattern length.
 *   @post: Out The post-pattern length.
 */
bool pat_pre(const char *str, uint32_t *pre, uint32_t *post)
{
	const char *find;

	find = strchr(str, '%');
	if((find == NULL) || (strrchr(str, '%') != find))
		return false;

	*pre = find - str;
	*post = strlen(str) - *pre - 1;
	return true;
}

struct val_t *fn_pat(struct val_t **args, uint32_t cnt, struct loc_t loc)
{
	uint32_t pre, post;
	struct buf_t buf;
	struct val_t *val, *ret, **iter;
	char *pat, *repl;

	if(cnt != 3)
		loc_err(loc, "Function `.pat` requires 2 arguments.");

	iter = &ret;
	pat = val_str(args[1], loc);
	repl = val_str(args[2], loc);

	if(!pat_pre(pat, &pre, &post))
		loc_err(loc, "Function `.pat` requires patterns as arguments (must contain a single '%').");
	//find = strchr(pat, '%');
	//if(find == NULL)
		//loc_err(loc, "Function `.pat` requires patterns as arguments (must contain a single '%').");

	for(val = args[0]; val != NULL; val = val->next) {
		pat_pre(val->str, &pre, &post);
		//for(i = 0; pat[i] != '%'; i++) {
			//if(pat[i] == '\0')
		//}

		buf_new(32);
		buf_done(&buf);
		
		//str = find = NULL;
		//if(find || str);
	}

	free(pat);
	free(repl);
	*iter = NULL;

	return ret;
}

struct val_t *eval_str(const char **str, struct loc_t loc, struct ctx_t *ctx, struct env_t *env)
{
	switch(**str) {
	case '$':
		(*str)++;
		return eval_var(str, loc_off(loc, 1), ctx, env);

	case '"':
	case '\'':
		fatal("FIXME eval_str");

	default: {
		struct buf_t buf;

		if(!ch_str(**str))
			loc_err(loc, "Expected string.");

		buf = buf_new(32);

		do
			buf_ch(&buf, *(*str)++);
		while(ch_str(**str));

		return val_new(false, buf_done(&buf));
	} break;
	}
}

/**
 * Evaluate a variable.
 *   @str: Ref. The string.
 *   @loc: The location.
 *   @ctx: The context.
 *   @env: The environment.
 *   &returns: The value.
 */
struct val_t *eval_var(const char **str, struct loc_t loc, struct ctx_t *ctx, struct env_t *env)
{
	char id[256];
	struct bind_t *bind;
	struct val_t *val, *iter;
	const char *orig = *str;

	if((*str)[0] == '$')
		return val_new(false, "$$");
	else if((*str)[0] == '{') {
		(*str)++;
		val = eval_var(str, loc_off(loc, *str - orig), ctx, env);
		while(**str != '}') {
			uint32_t off, cnt;
			struct val_t **args;

			if(**str != '.')
				loc_err(loc_off(loc, *str - orig), "Expected '.' or '}'.");

			off = *str - orig;
			(*str)++;
			id[0] = '.';
			get_var(str, id + 1, loc);

			bind = env_get(env, id);
			if(bind == NULL)
				loc_err(loc_off(loc, off), "Unknown function '%s'.", id);

			args_init(&args, &cnt);
			args_add(&args, &cnt, val);

			str_trim(str);
			if(**str != '(')
				loc_err(loc_off(loc, *str - orig), "Expected '.' or '}'.");

			(*str)++;
			str_trim(str);
			if(**str != ')') {
				for(;;) {
					args_add(&args, &cnt, eval_str(str, loc_off(loc, *str - orig), ctx, env));
					str_trim(str);
					if(**str == ')')
						break;
					else if(**str != ',')
						loc_err(loc_off(loc, *str - orig), "Expected ',' or ')'.");

					(*str)++;
					str_trim(str);
				}
			}
			(*str)++;

			switch(bind->tag) {
			case val_v:
				loc_err(loc_off(loc, off), "Cannot call a value.");

			case func_v:
				val = bind->data.func(args, cnt, loc_off(loc, off));
				break;

			case ns_v:
				loc_err(loc_off(loc, off), "Cannot call a namespace.");
			}

			args_delete(args, cnt);
		}
		(*str)++;

		return val;
	}
	else {
		get_var(str, id, loc_off(loc, *str - orig));

		if(strcmp(id, "@") == 0) {
			if(ctx->gen == NULL)
				loc_err(loc_off(loc, *str - orig), "Variable `$@` can only be used within a recipe.");

			val = val_dup(ctx->gen);
			for(iter = val; iter != NULL; iter = iter->next)
				str_set(&val->str, str_fmt("$~%s", val->str));
			
			return val;
		}
		else if(strcmp(id, "^") == 0) {
			if(ctx->gen == NULL)
				loc_err(loc_off(loc, *str - orig), "Variable `$^` can only be used within a recipe.");

			return val_dup(ctx->dep);
		}
		else if(strcmp(id, "~") == 0) 
			fatal("STUBME"); // return ctx->dir ? val_new(false, strdup(ctx->dir)) : NULL;

		bind = env_get(env, id);
		if(bind == NULL)
			loc_err(loc, "Unknown variable '%s'.", id);

		switch(bind->tag) {
		case val_v: val = val_dup(bind->data.val); break;
		case ns_v: fatal("FIXME stub namespace binding");
		default: __builtin_unreachable();
		}
	}

	return val;
}


/**
 * Create an environment.
 *   @up: The parent environment
 */
struct env_t env_new(struct env_t *up)
{
	return (struct env_t){ map0_new((cmp_f)strcmp, (del_f)bind_delete), up };
}

/**
 * Delete an environment.
 *   @env: The environment.
 */
void env_delete(struct env_t env)
{
	map0_delete(env.map);
}


/**
 * Get a binding from an environment.
 *   @env: The environment.
 *   @id: The identifier.
 *   &returns: The binding if found.
 */
struct bind_t *env_get(struct env_t *env, const char *id)
{
	struct bind_t *bind;

	while(env != NULL) {
		bind = map0_get(env->map, id);
		if(bind != NULL)
			return bind;

		env = env->up;
	}

	return NULL;
}

/**
 * Add a binding to an environment.
 *   @env: The environment.
 *   @bind: The binding.
 */
void env_put(struct env_t *env, struct bind_t *bind)
{
	struct bind_t *cur;

	cur = map_rem(env->map, bind->id);
	if(cur != NULL)
		bind_delete(cur);

	map0_add(env->map, bind->id, bind);
}


/**
 * Create a job controller.
 *   @queue: The rule queue.
 *   @n: The maximum number of concurrent jobs.
 *   &returns: The controller.
 */
struct ctrl_t *ctrl_new(struct queue_t *queue, uint32_t n)
{
	uint32_t i;
	struct ctrl_t *ctrl;

	ctrl = malloc(sizeof(struct ctrl_t));
	ctrl->queue = queue;
	ctrl->cnt = n;
	ctrl->job = malloc(n * sizeof(struct job_t));

	for(i = 0; i < n; i++)
		ctrl->job[i].pid = -1;

	return ctrl;
}

/**
 * Delete a job controller.
 *   @ctrl: The controller.
 */
void ctrl_delete(struct ctrl_t *ctrl)
{
	free(ctrl->job);
	free(ctrl);
}


/**
 * Add a command to the controller.
 *   @ctrl: The controller.
 *   @rule: The rule.
 */
void ctrl_add(struct ctrl_t *ctrl, struct rule_t *rule)
{
	uint32_t i;

	if(rule->seq->head == NULL)
		return ctrl_done(ctrl, rule);

	for(i = 0; i < ctrl->cnt; i++) {
		if(ctrl->job[i].pid < 0)
			break;
	}

	if(i >= ctrl->cnt)
		fatal("Failed to start job.");

	ctrl->job[i].pid = os_exec(rule->seq->head);
	ctrl->job[i].rule = rule;
	ctrl->job[i].cmd = rule->seq->head->next;
}

/**
 * Determine if there is an available job.
 *   @ctrl: The controller.
 *   &returns: True if available.
 */
bool ctrl_avail(struct ctrl_t *ctrl)
{
	uint32_t i;

	for(i = 0; i < ctrl->cnt; i++) {
		if(ctrl->job[i].pid < 0)
			return true;
	}

	return false;
}

/**
 * Determine if there is at least one busy job.
 *   @ctrl: The controller.
 *   &returns: True if busy.
 */
bool ctrl_busy(struct ctrl_t *ctrl)
{
	uint32_t i;

	for(i = 0; i < ctrl->cnt; i++) {
		if(ctrl->job[i].pid >= 0)
			return true;
	}

	return false;
}

/**
 * Wait for a job to complete.
 *   @ctrl: The controller.
 */
void ctrl_wait(struct ctrl_t *ctrl)
{
	uint32_t i;
	int pid;

	pid = os_wait();

	for(i = 0; i < ctrl->cnt; i++) {
		if(ctrl->job[i].pid == pid)
			break;
	}

	if(i >= ctrl->cnt)
		return;

	if(ctrl->job[i].cmd != NULL) {
		ctrl->job[i].pid = os_exec(ctrl->job[i].cmd);
		ctrl->job[i].cmd = ctrl->job[i].cmd->next;
	}
	else {
		ctrl->job[i].pid = -1;
		ctrl_done(ctrl, ctrl->job[i].rule);
	}
}

/**
 * Done with a rule, adding rules to queue as ready.
 *   @ctrl: The controller.
 *   @rule: The rule.
 */
void ctrl_done(struct ctrl_t *ctrl, struct rule_t *rule)
{
	struct edge_t *edge;
	struct target_t *target;
	struct target_iter_t iter;

	iter = target_iter(rule->gens);
	while((target = target_next(&iter)) != NULL) {
		target->mtime = -1;

		for(edge = target->edge; edge != NULL; edge = edge->next) {
			edge->rule->edges--;
			if(edge->rule->edges == 0)
				queue_add(ctrl->queue, edge->rule);
		}
	}
}


/**
 * Set structure.
 *   @elem: The element.
 */
struct set_t {
	struct elem_t *elem;
};

/**
 * Element structure.
 *   @ref: The reference.
 *   @next: The next element.
 */
struct elem_t {
	void *ref;
	struct elem_t *next;
};


/**
 * Create a set.
 *   &returns: The set.
 */
struct set_t *set_new(void)
{
	struct set_t *set;

	set = malloc(sizeof(struct set_t));
	set->elem = NULL;

	return set;
}

/**
 * Delete a set.
 *   @set; The set.
 *   @del: The element deletion function.
 */
void set_delete(struct set_t *set, void(*del)(void *))
{
	struct elem_t *elem, *tmp;

	elem = set->elem;
	while(elem != NULL) {
		elem = (tmp = elem)->next;
		
		if(del != NULL)
			del(tmp->ref);

		free(tmp);
	}

	free(set);
}


/**
 * Add an element to a map.
 *   @map: The map.
 *   @ref: The reference.
 */
void set_add(struct set_t *set, void *ref)
{
	struct elem_t *elem;

	elem = malloc(sizeof(struct elem_t));
	elem->ref = ref;

	elem->next = set->elem;
	set->elem = elem;
}

/**
 * Create a list.
 *   @del: The value deletion function.
 *   &returns: The list.
 */
struct list_t *list_new(void(*del)(void *))
{
	struct list_t *list;

	list = malloc(sizeof(struct list_t));
	list->head = NULL;
	list->tail = &list->head;
	list->del = del;

	return list;
}

/**
 * Delete a list.
 *   @list: The list.
 */
void list_delete(struct list_t *list)
{
	struct link_t *link, *tmp;

	link = list->head;
	while(link != NULL) {
		link = (tmp = link)->next;
		
		if(list->del != NULL)
			list->del(tmp->val);

		free(tmp);
	}

	free(list);
}


/**
 * Add a value to a list.
 *   @list: The list.
 *   @val: The value.
 */
void list_add(struct list_t *list, void *val)
{
	struct link_t *link;

	link = malloc(sizeof(struct link_t));
	link->val = val;
	link->next = NULL;

	*list->tail = link;
	list->tail = &link->next;
}


struct map1_t *map1_new(void (*cmp)(const void *, const void *), void (*del)(void *))
{
	return NULL;
}



#if 0
/**
 * Create a map.
 *   @init: The initial size.
 *   &returns: The map.
 */
struct map_t *map_new(uint32_t init)
{
	struct map_t *map;

	map = malloc(sizeof(struct map_t));
	map->cnt = 0;
	map->sz = init;
	map->arr = malloc(init * sizeof(void *));
	memset(map->arr, 0x00, init * sizeof(void *));

	map->alt = NULL;
	map->idx = 0;

	return map;
}

/**
 * Delete a map.
 *   @map: The map.
 */
void map_delete(struct map_t *map, size_t off, void(*del)(void *))
{
	uint32_t i;

	for(i = 0; i < map->sz; i++)
		ent_clear(map->arr[i], off, del);

	if(map->alt != NULL)
		map_delete(map->alt, off, del);

	free(map->arr);
	free(map);
}


/**
 * Retrieve an entity from the map.
 *   @map: The map.
 *   @hash: The hash.
 *   @ref: The reference.
 *   @off: The offset.
 *   @cmp: The comparison function.
 *   &returns: The entity if found, null if not found.
 */
struct ent_t *map_get(struct map_t *map, uint64_t hash, void *ref, ssize_t off, bool(*cmp)(const void *, const void *))
{
	struct ent_t *ent;

	for(ent = map->arr[hash % map->sz]; ent != NULL; ent = ent->next) {
		if((hash == ent->hash) && cmp(ref, (void *)ent - off))
			return ent;
	}

	return NULL;
}


/**
 * Add an entity to the map.
 *   @map: The map.
 *   @ent: The entity.
 *   @off: The offset.
 *   @cmp: The comparison function.
 *   &returns: True if added, false if already in map.
 */
bool map_add(struct map_t *map, struct ent_t *ent, ssize_t off, bool(*cmp)(const void *, const void *))
{
	struct ent_t *iter, **ref;

	ref = &map->arr[ent->hash % map->sz];
	for(iter = *ref; iter != NULL; iter = iter->next) {
		if((iter->hash == ent->hash) && cmp(ref, (void *)ent - off))
			return false;
	}

	ent->next = *ref;
	*ref = ent;

	return true;
}


/**
 * Determine if two entities are equal.
 *   @lhs: The left-hand side.
 *   @rhs: The right-hand side.
 *   @fn: The comparison function.
 *   &returns: True if equal, false otherwise.
 */
bool ent_equal(const struct ent_t *lhs, const struct ent_t *rhs, size_t off, bool(*fn)(void *,void *))
{
	return fn((void *)lhs - off, (void *)rhs - off);
}


/**
 * Create an entity.
 *   @hash: The hash.
 *   &returns: The entity.
 */
struct ent_t ent_new(uint64_t hash)
{
	return (struct ent_t){ hash, NULL };
}

/**
 * Delete an entity.
 *   @ent: The entity.
 *   @off: The offset to the parent.
 *   @del: The deletion callback.
 */
void ent_delete(struct ent_t *ent, size_t off, void(*del)(void *))
{
	del((void*)ent - off);
}

/**
 * Clear a list of entities.
 *   @ent: The entity list.
 *   @off: The offset to the parent.
 *   @del: The deletion callback.
 */
void ent_clear(struct ent_t *ent, size_t off, void(*del)(void *))
{
	struct ent_t *tmp;

	while(ent != NULL) {
		ent = (tmp = ent)->next;
		ent_delete(tmp, off, del);
	}
}
#endif


/**
 * Create a new namespace.
 *   @id: Optional. Consumed. The identifier.
 *   @up: Optional. The parent namespace.
 *   &returns: The namespace.
 */
struct ns_t *ns_new(char *id, struct ns_t *up)
{
	struct ns_t *ns;

	ns = malloc(sizeof(struct ns_t));
	ns->id = id;
	ns->up = up;
	ns->next = NULL;
	ns->bind = NULL;

	return ns;
}

/**
 * Delete a namespace.
 *   @ns: The namespace.
 */
void ns_delete(struct ns_t *ns)
{
	struct bind_t *bind;

	while(ns->bind != NULL) {
		ns->bind = (bind = ns->bind)->next;
		bind_delete(bind);
	}

	if(ns->id != NULL)
		free(ns->id);

	free(ns);
}


/**
 * Add a binding to the namespace.
 *   @ns: The namespace.
 *   @bind: The binding.
 */
void ns_add(struct ns_t *ns, struct bind_t *bind)
{
	if(bind->id == NULL) {
		bind->next = ns->bind;
		ns->bind = bind;
	}
	else {
		cli_err("FIXME STUB SLKFJSDLFJ");
	}
}


/**
 * Find a binding.
 *   @ns: The namespace.
 *   @id: The identifier.
 *   &returns: The binding if found, null otherwise.
 */
struct bind_t *ns_find(struct ns_t *ns, const char *id)
{
	return *ns_lookup(ns, id);
}

/**
 * Lookup a variable only on the current level.
 *   @ns: The namespace.
 *   @id: The identifier.
 *   &returns: The binding if found, point to end of list otherwise.
 */
struct bind_t **ns_lookup(struct ns_t *ns, const char *id)
{
	struct bind_t **bind;

	bind = &ns->bind;
	while(*bind != NULL) {
		if(strcmp((*bind)->id, id) == 0)
			break;

		bind = &(*bind)->next;
	}

	return bind;
}


/**
 * Create a rule.
 *   @id: Consumed. Optional. The identifier.
 *   @gens: Consumed. The targets.
 *   @deps: Consumed. The dependencies.
 *   @cmds: Consumed. The commands.
 *   &returns: The rule.
 */
struct rule_t *rule_new(char *id, struct target_list_t *gens, struct target_list_t *deps, struct seq_t *seq)
{
	struct rule_t *rule;

	rule = malloc(sizeof(struct rule_t));
	*rule = (struct rule_t){ id, gens, deps, seq, false, 0 };

	return rule;
}

/**
 * Delete a rule.
 *   @rule: The rule.
 */
void rule_delete(struct rule_t *rule)
{
	target_list_delete(rule->gens);
	target_list_delete(rule->deps);
	seq_delete(rule->seq);
	free(rule);
}


/**
 * Retrieve an iterator to the rule list.
 *   @list: The list.
 *   &returns: The iterator.
 */
struct rule_iter_t rule_iter(struct rule_list_t *list)
{
	return (struct rule_iter_t){ list->inst };
}

/**
 * Retrieve the next rule.
 *   @iter: The iterater.
 *   &returns: The rule or null.
 */
struct rule_t *rule_next(struct rule_iter_t *iter)
{
	struct rule_t *rule;

	if(iter->inst == NULL)
		return NULL;

	rule = iter->inst->rule;
	iter->inst = iter->inst->next;
	return rule;
}


/**
 * Create a list of rules.
 *   &returns: The list.
 */
struct rule_list_t *rule_list_new(void)
{
	struct rule_list_t *list;

	list = malloc(sizeof(struct rule_list_t));
	*list = (struct rule_list_t){ NULL };

	return list;
}

/**
 * Delete a list of rules.
 *   @list: The list.
 */
void rule_list_delete(struct rule_list_t *list)
{
	struct rule_inst_t *inst, *tmp;

	inst = list->inst;
	while(inst != NULL) {
		inst = (tmp = inst)->next;
		rule_delete(tmp->rule);
		free(tmp);
	}

	free(list);
}


/**
 * Add a rule to the list.
 *   @list: The list.
 *   @rule: The rule.
 */
void rule_list_add(struct rule_list_t *list, struct rule_t *rule)
{
	struct rule_inst_t *inst;

	inst = malloc(sizeof(struct rule_inst_t));
	inst->rule = rule;

	inst->next = list->inst;
	list->inst = inst;
}


/**
 * Queue structure.
 *   @head, tail: The head and tail items.
 */
struct queue_t {
	struct item_t *head, **tail;
};

/**
 * Item structure.
 *   @rule: The rule.
 *   @next: The next item.
 */
struct item_t {
	struct rule_t *rule;
	struct item_t *next;
};


/**
 * Create a queue.
 *   &returns: The queue.
 */
struct queue_t *queue_new(void)
{
	struct queue_t *queue;

	queue = malloc(sizeof(struct queue_t));
	*queue = (struct queue_t){ NULL, &queue->head };

	return queue;
}

/**
 * Delete a queue.
 *   @queue: The queue.
 */
void queue_delete(struct queue_t *queue)
{
	free(queue);
}


/**
 * Recursivly add rules to a queue.
 *   @queue: The queue.
 *   @rule: The root rule.
 */
void queue_recur(struct queue_t *queue, struct rule_t *rule)
{
	uint32_t cnt = 0;
	struct target_t *target;
	struct target_iter_t iter;

	if(rule->add)
		return;

	rule->add = true;
	iter = target_iter(rule->deps);
	while((target = target_next(&iter)) != NULL) {
		if(target->rule != NULL) {
			cnt++;
			queue_recur(queue, target->rule);
		}
	}

	if(cnt == 0)
		queue_add(queue, rule);
	else
		rule->edges = cnt;
}

/**
 * Add a rule to the queue.
 *   @queue: The queue.
 *   @rule: the rule.
 */
void queue_add(struct queue_t *queue, struct rule_t *rule)
{
	struct item_t *item;

	item = malloc(sizeof(struct item_t));
	item->rule = rule;
	item->next = NULL;

	*queue->tail = item;
	queue->tail = &item->next;
}

/**
 * Remove a rule from the queue.
 *   @queue: The queue.
 *   &returnss: The rule or null if no rules are available.
 */
struct rule_t *queue_rem(struct queue_t *queue)
{
	struct item_t *item;
	struct rule_t *rule;

	item = queue->head;
	if(item == NULL)
		return NULL;

	queue->head = item->next;
	if(queue->head == NULL)
		queue->tail = &queue->head;

	rule = item->rule;
	free(item);

	return rule;
}


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
	return ch_alnum(ch) || (strchr("~/._-+", ch) != NULL);
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
 * Create a target.
 *   @spec: The special flag.
 *   @path: Consumed. The file path.
 *   &returns: The target.
 */
struct target_t *target_new(bool spec, char *path)
{
	struct target_t *target;

	target = malloc(sizeof(struct target_t));
	*target = (struct target_t){ path, spec ? FLAG_SPEC : 0, -1, NULL, NULL };

	return target;
}

/**
 * Delete a target.
 *   @target: The target.
 */
void target_delete(struct target_t *target)
{
	struct edge_t *edge, *tmp;

	edge = target->edge;
	while(edge != NULL) {
		edge = (tmp = edge)->next;
		free(tmp);
	}

	free(target->path);
	free(target);
}


/**
 * Retrieve the target modification time, caching it as needed.
 *   @target: The target.
 *   &returns: The modification time.
 */
int64_t target_mtime(struct target_t *target)
{
	if(target->mtime < 0)
		target->mtime = os_mtime(target->path);

	return target->mtime;
}


/**
 * Connect a target to a rule.
 *   @target: The target.
 *   @rule: The rule.
 */
void target_conn(struct target_t *target, struct rule_t *rule)
{
	struct edge_t *edge;

	edge = malloc(sizeof(struct edge_t));
	edge->rule = rule;
	edge->next = target->edge;
	target->edge = edge;
}


/**
 * Retrieve an iterator to the target list.
 *   @list: The list.
 *   &returns: The iterator.
 */
struct target_iter_t target_iter(struct target_list_t *list)
{
	return (struct target_iter_t){ list->inst };
}

/**
 * Retrieve the next target.
 *   @iter: The iterater.
 *   &returns: The target or null.
 */
struct target_t *target_next(struct target_iter_t *iter)
{
	struct target_t *target;

	if(iter->inst == NULL)
		return NULL;

	target = iter->inst->target;
	iter->inst = iter->inst->next;
	return target;
}


/**
 * Create a target list.
 *   &returns: The list.
 */
struct target_list_t *target_list_new(void)
{
	struct target_list_t *list;

	list = malloc(sizeof(struct target_list_t));
	*list = (struct target_list_t){ NULL };

	return list;
}

/**
 * Destroy a target list.
 *   @list: The list.
 */
void target_list_delete(struct target_list_t *list)
{
	struct target_inst_t *inst, *tmp;

	inst = list->inst;
	while(inst != NULL) {
		inst = (tmp = inst)->next;
		free(tmp);
	}

	free(list);
}


/**
 * Retrieve the list length.
 *   @list: The list.
 *   &returns: The length.
 */
uint32_t target_list_len(struct target_list_t *list)
{
	uint32_t n = 0;
	struct target_inst_t *inst;

	for(inst = list->inst; inst != NULL; inst = inst->next)
		n++;

	return n;
}

/**
 * Check if the list contains a target.
 *   @list: The list.
 *   @target: The target.
 *   &returns: True if target exists within the list.
 */
bool target_list_contains(struct target_list_t *list, struct target_t *target)
{
	struct target_inst_t *inst;

	for(inst = list->inst; inst != NULL; inst = inst->next) {
		if(inst->target == target)
			return true;
	}

	return false;
}

/**
 * Add a target to the list.
 *   @list: The list.
 *   @target: The target.
 */
void target_list_add(struct target_list_t *list, struct target_t *target)
{
	struct target_inst_t *inst;

	inst = malloc(sizeof(struct target_inst_t));
	inst->target = target;

	inst->next = list->inst;
	list->inst = inst;
}

struct target_t *target_list_find(struct target_list_t *list, bool spec, const char *path)
{
	struct target_inst_t *inst;

	for(inst = list->inst; inst != NULL; inst = inst->next) {
		if((strcmp(inst->target->path, path) == 0))
			return inst->target;
	}

	return NULL;
}


/**
 * Create a target map.
 *   &returns: The map.
 */
struct map_t *map_new(void)
{
	struct map_t *map;

	map = malloc(sizeof(struct map_t));
	map->ent = NULL;

	return map;
}

/**
 * Delete a target map.
 *   @map: The map.
 */
void map_delete(struct map_t *map)
{
	struct ent_t *ent, *tmp;

	ent = map->ent;
	while(ent != NULL) {
		ent = (tmp = ent)->next;
		target_delete(tmp->target);
		free(tmp);
	}

	free(map);
}


/**
 * Retrieve a target from a map.
 *   @map: The map.
 *   @spec: The special flag.
 *   @path: The path.
 *   &returns: The target or null if not found.
 */
struct target_t *map_get(struct map_t *map, bool spec, const char *path)
{
	struct ent_t *ent;

	for(ent = map->ent; ent != NULL; ent = ent->next) {
		if(strcmp(ent->target->path, path) == 0)
			return ent->target;
	}

	return NULL;
}

/**
 * Add a target to map.
 *   @map: The map.
 *   @target: Consumed. The target.
 */
void map_add(struct map_t *map, struct target_t *target)
{
	struct ent_t *ent;

	ent = malloc(sizeof(struct ent_t));
	ent->target = target;

	ent->next = map->ent;
	map->ent = ent;
}

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
			pipe(pair);
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
