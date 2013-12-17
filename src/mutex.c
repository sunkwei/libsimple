#include <stdlib.h>
#include <malloc.h>
#ifdef WIN32
#  include <Windows.h>
#else
#  include <pthread.h>
#endif //
#include "../include/simple/mutex.h"

struct mutex_t
{
#ifdef WIN32
	CRITICAL_SECTION cs;
#else
	pthread_mutex_t cs;
#endif //
};

mutex_t *simple_mutex_create()
{
	mutex_t *m = (mutex_t*)malloc(sizeof(mutex_t));

#ifdef WIN32
	InitializeCriticalSection(&m->cs);
#else
	do {
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&m->cs, &attr);
	} while (0);
#endif // os

	return m;
}

void simple_mutex_destroy(mutex_t *m)
{
#ifdef WIN32
	DeleteCriticalSection(&m->cs);
#else
	pthread_mutex_destroy(&m->cs);
#endif // os

	free(m);
}

void simple_mutex_lock(mutex_t *m)
{
#ifdef WIN32
	EnterCriticalSection(&m->cs);
#else
	pthread_mutex_lock(&m->cs);
#endif // os
}

void simple_mutex_unlock(mutex_t *m)
{
#ifdef WIN32
	LeaveCriticalSection(&m->cs);
#else
	pthread_mutex_unlock(&m->cs);
#endif // os
}
