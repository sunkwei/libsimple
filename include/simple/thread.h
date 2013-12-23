#ifndef simple_thread__hh
#define simple_thread__hh

#include "mutex.h"
#include "semaphore.h"

typedef struct thread_t thread_t;

/** 工作线程函数 */
typedef int (*simple_thread_proc)(void *opaque);

thread_t *simple_thread_create(simple_thread_proc proc, void *opaque);
void simple_thread_join(thread_t *th);

/** 与工作线程的交互 

	rpl = simple_thread_req(req); 将 req 传递给工作线程，阻塞，直到工作线程调用 simple_thread_reply(rpl) 返回

	一般工作线程应该：
		while (!quit) {
			req = simple_thread_getreq();
			if (req ....) {
				... 处理
				simple_thread_reply(rpl);
			}
		}
	
 */
void *simple_thread_req(thread_t *th, void *req);

// 阻塞timeout_ms，直到有 req，如果超时，返回0，如果 req 有效，返回 1
int simple_thread_chkreq(thread_t *th, void **req, int timeout_ms);	
void *simple_thread_getreq(thread_t *th);	// 阻塞，知道有 req
void simple_thread_reply(thread_t *th, void *rpl);

// sleep(ms)
void simple_thread_msleep(int ms);

#endif // thread.h
