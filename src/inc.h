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
 * shorter type names
 */
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;

/*
 * structure prototypes
 */
struct cmd_t;
struct ctx_t;
struct env_t;
struct list_t;
struct queue_t;
struct ns_t;
struct rd_t;
struct rule_t;
struct seq_t;
struct set_t;
struct target_t;
struct target_list_t;

struct loc_t;
struct tok_t;
struct val_t;

typedef int(*cmp_f)(const void *, const void *);
typedef void(*del_f)(void *);

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

void os_exec(struct val_t *val);
int64_t os_mtime(const char *path);
void os_mkdir(const char *path);

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
 *   @dir: The selected directory.
 */
struct opt_t {
	bool force;
	const char *dir;
};

/**
 * Context structure.
 *   @opt: The options.
 *   @map: The target map.
 *   @rules: The set of rules.
 *   @gen, dep: The generator and depedency values.
 *   @str, dir: The working string and selected directory.
 *   @len, max: The length and maximum.
 *   @gen, deps: The generated and dependency targets.
 */
struct ctx_t {
	const struct opt_t *opt;

	struct map_t *map;
	struct rule_list_t *rules;

	struct val_t *gen, *dep;
	char *str, *dir;
	uint32_t len, max;

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

const char *ctx_str(struct ctx_t *ctx, struct ns_t *ns, struct tok_t *tok);


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
 *   @val: The value.
 *   @next: The next value.
 */
struct cmd_t {
	struct val_t *val;
	struct cmd_t *next;
};

/*
 * sequence declarations
 */
struct seq_t *seq_new(void);
void seq_delete(struct seq_t *seq);

void seq_add(struct seq_t *seq, struct val_t *val);


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
 * reader declarations
 */
struct rd_t *rd_open(const char *path);
void rd_close(struct rd_t *rd);

char rd_ch(struct rd_t *rd);
struct tok_t *rd_tok(struct rd_t *rd);
struct tok_t *rd_top(struct rd_t *rd);
struct tok_t *rd_get(struct rd_t *rd, int idx);
struct tok_t *rd_adv(struct rd_t *rd, uint32_t cnt);

void rd_err(struct rd_t *rd, const char *fmt, ...) __attribute__((noreturn));

/*
 * location declarations
 */
void loc_err(struct loc_t loc, const char *fmt, ...) __attribute__((noreturn));


/**
 * Token structure.
 *   @id, len: The identifier and length.
 *   @buf: The string buffer.
 *   @loc: The location information.
 *   @next: The next token.
 */
struct tok_t {
	uint32_t id;
	char *str;
	struct loc_t loc;

	struct tok_t *next;
};

/*
 * token definitions
 */
#define TOK_ID   (0x1000)
#define TOK_STR  (0x1001)
#define TOK_STR2 (0x1002)
#define TOK_EOF  (0xFFFF)

/*
 * token declaratinos
 */
struct tok_t *tok_new(uint32_t id, char *str, struct loc_t loc);
void tok_delete(struct tok_t *tok);

void tok_err(struct tok_t *tok, const char *fmt, ...);


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

void val_final(struct val_t *val, const char *dir);


/**
 * Binding structure.
 *   @id: The identifier.
 *   @tag: The tag.
 *   @data: The data.
 *   @loc: The location.
 *   @next: The next binding.
 */
enum bind_e { val_v, rule_v, ns_v };
union bind_u { struct val_t *val; struct syn_t *syn; };
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

struct bind_t *bind_val(char *id, struct val_t *val);
//struct bind_t *bind_rule(char *id, struct rule_t *rule);

/*
 * string converstion declarations
 */
char *get_str(const char **str, struct loc_t loc);
void get_var(const char **str, char *var, struct loc_t loc);

/*
 * string declarations
 */
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
char *buf_done(struct buf_t *buf);

void buf_ch(struct buf_t *buf, char ch);
void buf_mem(struct buf_t *buf, const char *mem, u32 len);
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

/*
 * string type declarations
 */
bool is_var(const char *str);


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
 * Statement structure.
 *   @tag: The tag.
 *   @data: The data.
 *   @loc: The location.
 *   @next: The next statement.
 */
enum stmt_e { assign_v, syn_v, dir_v, print_v };
union stmt_u { struct assign_t *assign; struct syn_t *syn; struct dir_t *dir; struct print_t *print; };
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

u32 imm_len(struct imm_t *imm);


/**
 * Raw string structure.
 *   @spec, quote: Special and quote flags.
 *   @str: The string.
 *   @loc: The location.
 *   @next: The next raw string.
 */
struct raw_t {
	bool spec, quote;

	char *str;
	struct loc_t loc;

	struct raw_t *next;
};

/*
 * raw value declarations
 */
struct raw_t *raw_new(bool spec, bool quote, char *str, struct loc_t loc);
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
