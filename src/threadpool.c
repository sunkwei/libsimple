#include <stdio.h>
#include <stdlib.h>
#include "../include/simple/thread.h"
#include "../include/simple/list.h"
#include "../include/simple/threadpool.h"

struct task_t
{
	simple_task_handle handle;
	void *opaque;
};

typedef struct thread_list
{
	list_head head;
	thread_t *thread;
} thread_list;

typedef struct task_list
{
	list_head head;
	task_t *task;
} task_list;

struct thread_pool_t
{
	list_head threads;		// ���й����߳�
	list_head tasks;		// �������
	mutex_t *lock;			// ���������
	semaphore_t *sem;		// ���������ʱ�����ѹ����߳�
	int thread_cnt;			// �����߳���Ŀ
	int quit;				// �˳����
};

static int _thread_cnt(int cnt)
{
	return 8;
}

// �����̺߳���
static int _thread_proc(void *p)
{
	thread_pool_t *tp = (thread_pool_t*)p;
	task_t *task = 0;

	while (!tp->quit) {
		simple_sem_wait(tp->sem);
		
		if (tp->quit)
			break;

		simple_mutex_lock(tp->lock);
		if (!list_empty(&tp->tasks)) {
			task_list *t = (task_list*)tp->tasks.next;
			task = t->task;

			list_del((list_head*)t);
			free(t);
		}
		simple_mutex_unlock(tp->lock);

		if (task) {
			task->handle(task, task->opaque);
		}
	}

	return 0;
}

thread_pool_t *simple_thread_pool_create(int cnt)
{
	int i;
	thread_pool_t *p = (thread_pool_t*)malloc(sizeof(thread_pool_t));

	p->thread_cnt = _thread_cnt(cnt);

	p->sem = simple_sem_create(0);
	p->lock = simple_mutex_create();
	p->threads.next = &p->threads, p->threads.prev = &p->threads;
	p->tasks.prev = &p->tasks, p->tasks.next = &p->tasks;
	p->quit = 0;

	for (i = 0; i < p->thread_cnt; i++) {
		list_head *th = (list_head*)malloc(sizeof(thread_list));
		((thread_list*)th)->thread = simple_thread_create(_thread_proc, p);
		list_add(th, &p->threads);
	}

	return p;
}

void simple_thread_pool_destroy(thread_pool_t *p)
{
	list_head *pos, *n;
	int i;

	p->quit = 1;

	for (i = 0; i < p->thread_cnt; i++) {
		simple_sem_post(p->sem);
	}

	// join all threads
	list_for_each_safe(pos, n, &p->threads) {
		thread_list *th = (thread_list*)pos;
		simple_thread_join(th->thread);
		list_del(pos);
		free(th);
	}

	// free all pending tasks
	list_for_each_safe(pos, n, &p->tasks) {
		task_list *t = (task_list*)pos;
		list_del(pos);
		simple_task_destroy(t->task);
		free(t);
	}

	simple_mutex_destroy(p->lock);
	simple_sem_destroy(p->sem);
	free(p);
}

int simple_thread_pool_add_task(thread_pool_t *p, task_t *t)
{
	list_head *n = (list_head*)malloc(sizeof(task_list));
	((task_list*)n)->task = t;

	simple_mutex_lock(p->lock);
	list_add_tail(n, &p->tasks);
	simple_mutex_unlock(p->lock);

	simple_sem_post(p->sem);	// ֪ͨ���µ�����

	return 0;
}

task_t *simple_task_create(simple_task_handle h, void *opaque)
{
	task_t *t = (task_t*)malloc(sizeof(task_t));
	t->handle = h;
	t->opaque = opaque;

	return t;
}

void simple_task_destroy(task_t *t)
{
	free(t);
}
