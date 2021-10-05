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
struct cmd_t;
struct ctx_t;
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

/*
 * common declarations
 */
uint64_t hash64(uint64_t hash, const char *str);

char *str_fmt(const char *pat, ...);
void str_set(char **dst, char *src);

/*
 * backend declarations
 */
void print(const char *fmt, ...);
void printv(const char *fmt, va_list args);
void fatal(const char *fmt, ...) __attribute__((noreturn));

void os_exec(struct val_t *val);
int64_t os_mtime(const char *path);
void os_mkdir(const char *path);

void *mem_alloc(size_t sz);
void *mem_realloc(void *ptr, size_t sz);
void mem_free(void *ptr);

/*
 * set declarations
 */
struct set_t *set_new(void);
void set_delete(struct set_t *set, void(*del)(void *));

void set_add(struct set_t *set, void *ref);

/*
 * list declarations
 */
struct list_t *list_new(void);
void list_delete(struct list_t *list, void(*del)(void *));

void list_add(struct list_t *list, void *ref);


/*
 * parser declarations
 */
void par_top(struct rd_t *rd, struct ctx_t *ctx, struct ns_t *ns);
void par_stmt(struct rd_t *rd, struct ctx_t *ctx, struct ns_t *ns);
void par_rule(struct rd_t *rd, struct ctx_t *ctx, struct ns_t *ns);
struct seq_t *par_seq(struct rd_t *rd, struct ctx_t *ctx, struct ns_t *ns);
struct val_t *par_val(struct rd_t *rd, struct ctx_t *ctx, struct ns_t *ns);
void par_print(struct rd_t *rd, struct ctx_t *ctx, struct ns_t *ns);
void par_dir(struct rd_t *rd, struct ctx_t *ctx, struct ns_t *ns);
void par_assign(struct rd_t *rd, struct ctx_t *ctx, struct ns_t *ns);


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
 *   @str, dir: The working string and selected directory.
 *   @len, max: The length and maximum.
 */
struct ctx_t {
	const struct opt_t *opt;

	struct map_t *map;
	struct rule_list_t *rules;

	char *str, *dir;
	uint32_t len, max;
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
 *   @str: The string.
 *   @next: The next value.
 */
struct val_t {
	char *str;

	struct val_t *next;
};

/*
 * value declarations
 */
struct val_t *val_new(char *str);
void val_delete(struct val_t *val);
void val_clear(struct val_t *val);
uint32_t val_len(struct val_t *val);


/**
 * Binding structure.
 *   @id: The identifier.
 *   @tag: The tag.
 *   @data: The data.
 *   @next: The next binding.
 */
enum bind_e { val_v, rule_v, ns_v };
union bind_u { struct val_t *val; struct rule_t *rule; };
struct bind_t {
	char *id;

	enum bind_e tag;
	union bind_u data;

	struct bind_t *next;
};

/*
 * binding declarations
 */
struct bind_t *bind_new(char *id, enum bind_e tag, union bind_u data);
void bind_delete(struct bind_t *bind);

struct bind_t *bind_val(char *id, struct val_t *val);
struct bind_t *bind_rule(char *id, struct rule_t *rule);

/*
 * character declarations
 */
bool ch_space(int ch);
bool ch_alpha(int ch);
bool ch_num(int ch);
bool ch_alnum(int ch);
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
