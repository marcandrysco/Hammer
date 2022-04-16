#pragma once

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
 * structure prototypes
 */
struct ast_cmd_t;
struct ast_pipe_t;
struct cmd_t;
struct rt_ctx_t;
struct env_t;
struct imm_t;
struct list_t;
struct queue_t;
struct ns_t;
struct raw_t;
struct rd_t;
struct rule_t;
struct rt_obj_t;
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

typedef struct rt_obj_t(func_t)(struct rt_obj_t *args, uint32_t cnt, struct loc_t loc);

/*
 * common declarations
 */
uint64_t hash64(uint64_t hash, const char *str);

void memswap(void *lhs, void *rhs, size_t len);

/*
 * base declarations
 */
struct ast_block_t *ham_load(const char *path);

/*
 * backend declarations
 */
extern int64_t os_memcnt;

void print(const char *fmt, ...);
void printv(const char *fmt, va_list args);
void _fatal(const char *path, unsigned long line, const char *fmt, ...) __attribute__((noreturn));
void unreachable() __attribute__((noreturn));
#define fatal(...) _fatal(__FILE__, __LINE__, __VA_ARGS__)

void os_init(void);
int os_exec(struct cmd_t *cmd);
int os_wait(void);
int64_t os_mtime(const char *path);
void os_mkdir(const char *path);

/*
 * makedep declarations
 */
void mk_eval(struct rt_ctx_t *ctx, const char *path, bool strict);

void mk_trim(FILE *file, int *ch);
char *mk_str(FILE *file, int *ch);

bool mk_space(int ch);
bool mk_ident(int ch);

/*
 * argument declarations
 */
void args_init(struct rt_obj_t **args, uint32_t *cnt);
void args_add(struct rt_obj_t **args, uint32_t *cnt, struct rt_obj_t obj);
void args_delete(struct rt_obj_t *args, uint32_t cnt);


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
 * reference declarations
 */
struct target_t *rt_ref_new(bool spec, char *path);
void rt_ref_delete(struct target_t *ref);

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
 * Object structure.
 *   @tag: The tag.
 *   @data: The data.
 */
enum rt_obj_e { rt_null_v, rt_val_v, rt_env_v, rt_func_v };
union rt_obj_u { struct val_t *val; struct env_t *env; func_t *func; };
struct rt_obj_t {
	enum rt_obj_e tag;
	union rt_obj_u data;
};


/**
 * Context structure.
 *   @opt: The options.
 *   @map: The target map.
 *   @rules: The set of rules.
 *   @gen, dep: The generator and depedency values.
 *   @cur: The current rule.
 *   @gen, deps: The generated and dependency targets.
 */
struct rt_ctx_t {
	const struct opt_t *opt;

	struct map_t *map;
	struct rule_list_t *rules;

	struct rule_t *cur;

	struct target_list_t *gens, *deps;
};

/*
 * context declarations
 */
struct rt_ctx_t *ctx_new(const struct opt_t *opt);
void ctx_delete(struct rt_ctx_t *ctx);

void ctx_run(struct rt_ctx_t *ctx, const char **builds);

struct target_t *ctx_target(struct rt_ctx_t *ctx, bool spec, const char *path);
struct rule_t *ctx_rule(struct rt_ctx_t *ctx, const char *id, struct target_list_t *gens, struct target_list_t *deps);


/*
 * evaluation declarations
 */
struct val_t *rt_eval_val(struct imm_t *imm, struct rt_ctx_t *ctx, struct env_t *env, struct loc_t loc);
char *rt_eval_str(struct raw_t *raw, struct rt_ctx_t *ctx, struct env_t *env, struct loc_t loc);


/*
 * object declarations
 */
struct rt_obj_t rt_obj_new(enum rt_obj_e tag, union rt_obj_u data);
struct rt_obj_t rt_obj_dup(struct rt_obj_t obj);
void rt_obj_delete(struct rt_obj_t obj);
void rt_obj_set(struct rt_obj_t *dst, struct rt_obj_t src);

struct rt_obj_t rt_obj_null(void);
struct rt_obj_t rt_obj_val(struct val_t *val);
struct rt_obj_t rt_obj_env(struct env_t *env);
struct rt_obj_t rt_obj_func(func_t *func);

void rt_obj_add(struct rt_obj_t dst, struct rt_obj_t src, struct loc_t loc);


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
struct val_t **val_tail(struct val_t **val);


/**
 * Binding structure.
 *   @id: The identifier.
 *   @obj: The object.
 *   @loc: The location.
 *   @next: The next binding.
 */
struct bind_t {
	char *id;
	struct rt_obj_t obj;

	struct loc_t loc;

	struct bind_t *next;
};

/*
 * binding declarations
 */
struct bind_t *bind_new(char *id, struct rt_obj_t obj, struct loc_t loc);
void bind_delete(struct bind_t *bind);
void bind_erase(struct bind_t *bind);

void bind_set(struct bind_t **dst, struct bind_t *src);
void bind_reset(struct bind_t *bind, struct rt_obj_t obj, struct loc_t loc);

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
 * Include structure.
 *   @nest, opt: The nest and optional flags.
 *   @imm: The immediate value.
 */
struct ast_inc_t {
	bool nest, opt;
	struct imm_t *imm;
};

/*
 * include declarations
 */
struct ast_inc_t *ast_inc_new(bool nest, bool opt, struct imm_t *imm);
void ast_inc_delete(struct ast_inc_t *inc);

void ast_inc_eval(struct ast_inc_t *inc, struct rt_ctx_t *ctx, struct env_t *env, struct loc_t loc);


/**
 * Syntatic rule structure.
 *   @gen, dep: The generator and dependency values.
 *   @cmd: The list of commands.
 *   @in, out: The input and output redirect.
 *   @loc: The location.
 */
struct ast_rule_t {
	struct imm_t *gen, *dep;
	struct list_t *cmd;

	struct loc_t loc;
};

/*
 * syntax rule declarations
 */
struct ast_rule_t *ast_rule_new(struct imm_t *gen, struct imm_t *dep, struct loc_t loc);
void ast_rule_delete(struct ast_rule_t *syn);

void ast_rule_add(struct ast_rule_t *syn, struct imm_t *cmd);


/**
 * Binding structure.
 *   @id: The indetifier.
 *   @tag: The tag.
 *   @data: The data.
 *   @add: The append flag.
 */
enum ast_bind_e { ast_val_v, ast_func_v, ast_block_v };
union ast_bind_u { struct imm_t *val; void *func; struct ast_block_t *block; };
struct ast_bind_t {
	struct raw_t *id;

	enum ast_bind_e tag;
	union ast_bind_u data;
	bool add;
};

/*
 * binding declarations
 */
struct ast_bind_t *ast_bind_new(struct raw_t *id, enum ast_bind_e tag, union ast_bind_u data, bool add);
void ast_bind_delete(struct ast_bind_t *bind);

struct ast_bind_t *ast_bind_val(struct raw_t *id, struct imm_t *val, bool add);
struct ast_bind_t *ast_bind_block(struct raw_t *id, struct ast_block_t *block, bool add);


/**
 * Block structure.
 *   @stmt: The list of statements.
 */
struct ast_block_t {
	struct ast_stmt_t *stmt;
};

/*
 * block declarations
 */
struct ast_block_t *ast_block_new(void);
void ast_block_delete(struct ast_block_t *block);


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
 * Make dependency statement structure.
 *   @path: The path as an immediate
 *   @loc: The location.
 */
struct ast_mkdep_t {
	struct imm_t *path;

	struct loc_t loc;
};

/*
 * make dependency declarations
 */
struct ast_mkdep_t *ast_mkdep_new(struct imm_t *path, struct loc_t loc);
void ast_mkdep_delete(struct ast_mkdep_t *dep);
void ast_mkdep_eval(struct ast_mkdep_t *dep, struct rt_ctx_t *ctx, struct env_t *env);


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
	struct ast_stmt_t *body;

	struct loc_t loc;
};

struct loop_t *loop_new(char *id, struct imm_t *imm, struct ast_stmt_t *body, struct loc_t loc);
void loop_delete(struct loop_t *loop);


/**
 * Statement structure.
 *   @tag: The tag.
 *   @data: The data.
 *   @loc: The location.
 *   @next: The next statement.
 */
enum stmt_e { ast_bind_v, syn_v, loop_v, print_v, ast_mkdep_v, block_v, ast_inc_v };
union stmt_u { struct ast_bind_t *bind; struct ast_rule_t *syn; struct cond_t *conf; struct loop_t *loop; struct print_t *print; struct ast_mkdep_t *mkdep; struct ast_block_t *block; struct ast_inc_t *inc; };
struct ast_stmt_t {
	enum stmt_e tag;
	union stmt_u data;
	struct loc_t loc;

	struct ast_stmt_t *next;
};

/*
 * statement declarations
 */
struct ast_stmt_t *stmt_new(enum stmt_e tag, union stmt_u data, struct loc_t loc);
void stmt_delete(struct ast_stmt_t *stmt);
void stmt_clear(struct ast_stmt_t *stmt);

struct ast_stmt_t *ast_stmt_mkdep(struct ast_mkdep_t *mkdep, struct loc_t loc);
struct ast_stmt_t *ast_stmt_inc(struct ast_inc_t *inc, struct loc_t loc);


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
 *   @nrefs: THe number of references.
 *   @map: The variable mapping.
 *   @next: The next/parent environment.
 */
struct env_t {
	uint32_t nrefs;
	struct map0_t *map;

	struct env_t *next;
};

/*
 * evaluation declarations
 */
void eval_top(struct ast_block_t *block, struct rt_ctx_t *ctx);
void eval_block(struct ast_block_t *block, struct rt_ctx_t *ctx, struct env_t *env);
void eval_stmt(struct ast_stmt_t *stmt, struct rt_ctx_t *ctx, struct env_t *env);
struct rt_obj_t eval_imm(struct imm_t *imm, struct rt_ctx_t *ctx, struct env_t *env, struct loc_t loc);
struct rt_obj_t eval_raw(struct raw_t *raw, struct rt_ctx_t *ctx, struct env_t *env);
struct rt_obj_t eval_var(const char **str, struct loc_t loc, struct rt_ctx_t *ctx, struct env_t *env);

/*
 * environment declarations
 */
struct env_t *rt_env_new(struct env_t *next);
struct env_t *rt_env_dup(struct env_t *env);
void rt_env_delete(struct env_t *env);
void rt_env_clear(struct env_t *env);

struct bind_t *rt_env_lookup(struct env_t *env, const char *id);
struct bind_t *env_get(struct env_t *env, const char *id);
void env_put(struct env_t *env, struct bind_t *bind);
struct env_t **rt_env_tail(struct env_t **env);



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

int ctrl_exec(struct cmd_t *cmd);


/*
 * builtin function declarations
 */
struct rt_obj_t fn_sub(struct rt_obj_t *args, uint32_t cnt, struct loc_t loc);
struct rt_obj_t fn_pat(struct rt_obj_t *args, uint32_t cnt, struct loc_t loc);


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
