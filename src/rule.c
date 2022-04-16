#include "inc.h"


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
	if(rule->seq != NULL)
		seq_delete(rule->seq);

	target_list_delete(rule->gens);
	target_list_delete(rule->deps);
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
