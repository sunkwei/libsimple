#ifndef _thread_pool__hh
#define _thread_pool__hh

typedef struct thread_pool_t thread_pool_t;
typedef struct task_t task_t;

typedef void (*simple_task_handle)(task_t *t, void *opaque);

thread_pool_t *simple_thread_pool_create(int cnt);
void simple_thread_pool_destroy(thread_pool_t *p);

int simple_thread_pool_add_task(thread_pool_t *p, task_t *task);

task_t *simple_task_create(simple_task_handle proc, void *opaque);

/** һ���� simple_task_handle �ص��е��ã����� */
void simple_task_destroy(task_t *task);

#endif // threadpool.h
