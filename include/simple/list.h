#ifndef __list__hh
#define __list__hh

/** 抄袭 linux kernel 的 list.h
 */

#ifdef __cplusplus
extern "C" {
#endif // c++

typedef struct list_t list_t;

typedef struct list_head
{
	struct list_head *prev, *next;
} list_head;

__inline static void ____list_add(list_head *n, list_head *prev, list_head *next)
{
	next->prev = n;
	n->next = next;
	n->prev = prev;
	prev->next = n;
}

__inline static void ____list_splice(list_head *l, list_head *prev, list_head *next)
{
	list_head *first = l->next, *last = l->prev;
	
	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

__inline static void ____list_exchange(list_head *n1, list_head *n1prev, list_head *n1next,
									   list_head *n2, list_head *n2prev, list_head *n2next)
{
	n1->prev = n2prev, n2prev->next = n1;
	n1->next = n2next, n2next->prev = n1;
	n2->prev = n1prev, n1prev->next = n2;
	n2->next = n1next, n1next->prev = n2;
}

/////////////////////// 以下为常用 api //////////////////////


/** 初始化 */
#define list_init(_head) { (_head)->next = (_head)->prev = (_head); }

/** 是否空 */
#define list_empty(_head) (_head == (_head)->next)


/** 添加 node 到 head 的列表 */
#define list_add(node, head) { ____list_add(node, head, (head)->next); }
#define list_add_tail(node, head) { ____list_add(node, (head)->prev, head); }

/** 删除 node */
#define list_del(_node) { \
	list_head *next = (_node)->next, *prev = (_node)->prev; \
	(_node)->prev->next = next;	\
	(_node)->next->prev = prev;	\
}

/** for each */
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/** for each，可以安全删除 pos */
#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); pos = n, n = pos->next)

/** 合并 list 到 head */
#define list_splice(list, head)	{	\
	if (!list_empty(list)) ____list_splice(list, head, head->next); \
}

#define list_splice_tail(list, head) { \
	if (!list_empty(list)) ____list_splice(list, head->prev, head); \
}

/** 交换 node1, node2 */
#define list_exchange(node1, node2) \
	____list_exchange(node1, node1->prev, node1->next, node2, node2->prev, node2->next);

#ifdef __cplusplus
}
#endif // c++

#endif // list.h
