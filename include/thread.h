#ifndef simple_thread__hh
#define simple_thread__hh

#include "mutex.h"
#include "semaphore.h"

typedef struct thread_t thread_t;

/** 工作线程函数 */
typedef int (*simple_thread_proc)(void *opaque);

thread_t *simple_thread_create(simple_thread_proc proc, void *opaque);
void simple_thread_join(thread_t *th);

void simple_thread_msleep(int ms);

#endif // thread.h
