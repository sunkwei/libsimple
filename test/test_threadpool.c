#include <stdio.h>
#include <stdlib.h>
#include <simple/threadpool.h>
#include <simple/thread.h>

static int task_proc(task_t *task, void *opaque)
{
	int val = (int)opaque;

	fprintf(stderr, "%s:%d: val=%d\n", __FUNCTION__, 
			simple_thread_id(), val);

	simple_task_destroy(task); // 必须的！！

	return 0;
}

int main()
{
	int i;
	thread_pool_t *tp = simple_thread_pool_create(4);

	simple_thread_msleep(10);

	for (i = 0; i < 10; i++) {
		task_t *task = simple_task_create(task_proc, (void*)i);
		simple_thread_pool_add_task(tp, task);
	}

	simple_thread_msleep(1000);

	simple_thread_pool_destroy(tp);

	return 0;
}

