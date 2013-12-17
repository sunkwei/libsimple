#include <stdlib.h>
#include <malloc.h>
#ifdef WIN32
#  include <Windows.h>
#else
#  include <pthread.h>
#endif //
#include "../include/mutex.h"

struct mutex_t
{
#ifdef WIN32
	CRITICAL_SECTION cs;
#else
#endif //
};

mutex_t *simple_mutex_create()
{
	mutex_t *m = (mutex_t*)malloc(sizeof(mutex_t));

#ifdef WIN32
	InitializeCriticalSection(&m->cs);
#else
#endif // os

	return m;
}

void simple_mutex_destroy(mutex_t *m)
{
#ifdef WIN32
	DeleteCriticalSection(&m->cs);
#else
#endif // os

	free(m);
}

void simple_mutex_lock(mutex_t *m)
{
#ifdef WIN32
	EnterCriticalSection(&m->cs);
#else
#endif // os
}

void simple_mutex_unlock(mutex_t *m)
{
#ifdef WIN32
	LeaveCriticalSection(&m->cs);
#else
#endif // os
}
