#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef WIN32
#  include <Windows.h>
#  include <process.h>
#else
#  include <pthread.h>
#  include <unistd.h>
#  ifdef __MACH__
#  else
#    include <syscall.h>
#  endif
#endif // 
#include "../include/simple/thread.h"
#include "../include/simple/semaphore.h"

struct thread_t
{
#ifdef WIN32
	uintptr_t th;
#else
	pthread_t th;
#endif // os

	simple_thread_proc proc;
	void *opaque;

	semaphore_t *sem_req, *sem_rpl;
	mutex_t *mut;
	void *req, *rpl;
};

#ifdef WIN32
static unsigned __stdcall _win_proc(void *p)
{
	thread_t *tp = (thread_t*)p;
	return tp->proc(tp->opaque);
}
#else
static void* _posix_proc(void *p)
{
	thread_t *tp = (thread_t*)p;
	tp->proc(tp->opaque);
	return 0;
}
#endif // os

int simple_thread_id()
{
#ifdef WIN32
	return GetCurrentThreadId();
#else
#  ifdef __MACH__
	return pthread_mach_thread_np(pthread_self());
#  else
	return syscall(SYS_gettid);
#  endif
#endif // os
}

thread_t *simple_thread_create(simple_thread_proc proc, void *opaque)
{
	thread_t *t = (thread_t*)malloc(sizeof(thread_t));
	t->proc = proc;
	t->opaque = opaque;

	t->mut = simple_mutex_create();
	t->sem_req = simple_sem_create(0);
	t->sem_rpl = simple_sem_create(0);

#ifdef WIN32
	t->th = _beginthreadex(0, 0, _win_proc, t, 0, 0);
#else
	pthread_create(&t->th, 0, _posix_proc, t);
#endif // os
	return t;
}

void simple_thread_join(thread_t *p)
{
#ifdef WIN32
	WaitForSingleObject((HANDLE)p->th, -1);
	CloseHandle((HANDLE)p->th);
#else
	void *rc;
	pthread_join(p->th, &rc);
#endif // os

	simple_mutex_destroy(p->mut);
	simple_sem_destroy(p->sem_req);
	simple_sem_destroy(p->sem_rpl);

	free(p);
}

void simple_thread_msleep(int ms)
{
#ifdef WIN32
	Sleep(ms);
#else
	usleep(ms*1000);
#endif // os
}

void *simple_thread_req(thread_t *th, void *req)
{
	void *rpl;

	simple_mutex_lock(th->mut);
	th->req = req;
	simple_sem_post(th->sem_req);	// valid
	simple_sem_wait(th->sem_rpl);	// wait rpl to valid
	rpl = th->rpl;
	simple_mutex_unlock(th->mut);

	return rpl;
}

void *simple_thread_getreq(thread_t *th)
{
	simple_sem_wait(th->sem_req);
	return th->req;
}

int simple_thread_chkreq(thread_t *th, void **req, int timeout)
{
	if (simple_sem_wait_timeout(th->sem_req, timeout) == 0) {
		*req = th->req;
		return 1;
	}
	else {
		return 0;
	}
}

void simple_thread_reply(thread_t *th, void *rpl)
{
	th->rpl = rpl;
	simple_sem_post(th->sem_rpl);
}
