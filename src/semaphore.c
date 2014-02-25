#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
#  include <Windows.h>
#else
#  include <semaphore.h>
#  include <time.h>
#endif // os
#include "../include/simple/semaphore.h"

#ifdef __MACH__
#include <sys/time.h>
//clock_gettime is not implemented on OSX

#define CLOCK_REALTIME 0

static int clock_gettime(int id, struct timespec* t) 
{
	struct timeval now;
	int rv = gettimeofday(&now, NULL);
	if (rv) return rv;
	t->tv_sec  = now.tv_sec;
	t->tv_nsec = now.tv_usec * 1000;
	return 0;
}
#endif

struct semaphore_t
{
#ifdef WIN32
	HANDLE sem;
#else
	sem_t sem;
#endif // os
};

semaphore_t *simple_sem_create(int cnt)
{
	semaphore_t *s = (semaphore_t*)malloc(sizeof(semaphore_t));
#ifdef WIN32
	s->sem = CreateSemaphore(0, cnt, 0x7fffffff, 0);
#else
	sem_init(&s->sem, 0, cnt);
#endif // os
	return s;
}

void simple_sem_destroy(semaphore_t *s)
{
#ifdef WIN32
	CloseHandle(s->sem);
#else
	sem_destroy(&s->sem);
#endif // os
	free(s);
}

void simple_sem_post(semaphore_t *s)
{
#ifdef WIN32
	ReleaseSemaphore(s->sem, 1, 0);
#else
	sem_post(&s->sem);
#endif // os
}

void simple_sem_wait(semaphore_t *s)
{
#ifdef WIN32
	WaitForSingleObject(s->sem, -1);
#else
	sem_wait(&s->sem);
#endif // os
}

int simple_sem_wait_timeout(semaphore_t *s, int timeout_ms)
{
#ifdef WIN32
	if (WaitForSingleObject(s->sem, timeout_ms) == WAIT_OBJECT_0)
		return 0;
	else
		return -1;
#else
	struct timespec ts;
	int rc;

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += timeout_ms / 1000;
	ts.tv_nsec += (timeout_ms%1000)*1000000;
	if (ts.tv_nsec > 1000000000) {
		ts.tv_nsec -= 1000000000;
		ts.tv_sec++;
	}

	rc = sem_timedwait(&s->sem, &ts);
	if (rc == -1)
		return -1;
	else
		return 0;

#endif // os
}
