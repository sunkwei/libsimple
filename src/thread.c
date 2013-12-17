#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdint.h>
#ifdef WIN32
#  include <Windows.h>
#  include <process.h>
#else
#  include <pthread.h>
#  include <unistd.h>
#endif // 
#include "../include/thread.h"

struct thread_t
{
#ifdef WIN32
	uintptr_t th;
#else
	pthread_t th;
#endif // os
	simple_thread_proc proc;
	void *opaque;
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

thread_t *simple_thread_create(simple_thread_proc proc, void *opaque)
{
	thread_t *t = (thread_t*)malloc(sizeof(thread_t));
	t->proc = proc;
	t->opaque = opaque;
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
