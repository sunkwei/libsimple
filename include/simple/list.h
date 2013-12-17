#ifndef __list__hh
#define __list__hh

/** ��Ϯ linux kernel �� list.h
 */

#ifdef __cplusplus
extern "C" {
#endif // c++

typedef struct list_t list_t;

typedef struct list_head
{
	struct list_head *prev, *next;
} list_head;

#define __LIST_ADD(_node,_prev,_next) { \
		(_next)->prev = _node; \
		(_node)->next = _next; \
		(_node)->prev = _prev; \
		(_prev)->next = _node; \
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

/////////////////////// ����Ϊ���� api //////////////////////

#define LIST_HEAD_INIT(name) { &(name), &(name) }
#define LIST_HEAD(name) \
	list_head name = LIST_HEAD_INIT(name)


/** �Ƿ�� */
#define list_empty(_head) (_head == (_head)->next)


/** ��� node �� head ���б� */
#define list_add(node, head) \
	__LIST_ADD(node, head, (head)->next);

#define list_add_tail(node, head) \
	__LIST_ADD(node, (head)->prev, head);

/** ɾ�� node */
#define list_del(_node) { \
		list_head *next = (_node)->next, *prev = (_node)->prev; \
		(_node)->prev->next = next;	\
		(_node)->next->prev = prev;	\
	}

/** for each */
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/** for each�����԰�ȫɾ�� pos */
#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); pos = n, n = pos->next)

/** �ϲ� list �� head */
#define list_splice(list, head) \
	{	\
		if (!list_empty(list)) __LIST_SPLICE(list, head, head->next); \
	}

#define list_splice_tail(list, head) \
	{	\
		if (!list_empty(list)) __LIST_SPLICE(list, head->prev, head); \
	}

/** ���� node1, node2 */
#define list_exchange(node1, node2) \
	__LIST_EXCHANGE(node1, node1->prev, node1->next, node2, node2->prev, node2->next);

#ifdef __cplusplus
}
#endif // c++

#endif // list.h
