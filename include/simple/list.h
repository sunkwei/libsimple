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

#define __LIST_ADD(_new,_prev,_next) { \
		(_next)->prev = _new; \
		(_new)->next = _next; \
		(_new)->prev = _prev; \
		(_prev)->next = _new; \
	}

#define __LIST_SPLICE(_list, _prev, _next) \
	{	\
		list_head *first = (_list)->next;	\
		list_head *last = (_list)->prev;	\
		first->prev = _prev;	\
		(_prev)->next = first; \
		last->next = _next;	\
		(_next)->prev = last;	\
	}

#define __LIST_EXCHANGE(_n1, _n1prev, _n1next, _n2, _n2prev, _n2next) \
	{	\
		(_n1)->prev = _n2prev, (_n2prev)->next = _n1;	\
		(_n1)->next = _n2next, (_n2next)->prev = _n1;	\
		(_n2)->prev = _n1prev, (_n1prev)->next = _n2;	\
		(_n2)->next = _n1next, (_n1next)->prev = _n2;	\
	}

/////////////////////// 以下为常用 api //////////////////////

/** 初始化 */
#define list_init(_head) { (_head)->next = (_head)->prev = (_head); }

/** 是否空 */
#define list_empty(_head) (_head == (_head)->next)


/** 添加 node 到 head 的列表 */
#define list_add(node, head) __LIST_ADD(node, head, (head)->next);

#define list_add_tail(node, head) __LIST_ADD(node, (head)->prev, head);

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
#define list_splice(list, head) \
	{	\
		if (!list_empty(list)) __LIST_SPLICE(list, head, head->next); \
	}

#define list_splice_tail(list, head) \
	{	\
		if (!list_empty(list)) __LIST_SPLICE(list, head->prev, head); \
	}

/** 交换 node1, node2 */
#define list_exchange(node1, node2) \
	__LIST_EXCHANGE(node1, node1->prev, node1->next, node2, node2->prev, node2->next);

#ifdef __cplusplus
}
#endif // c++

#endif // list.h
