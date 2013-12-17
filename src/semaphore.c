#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#ifdef WIN32
#  include <Windows.h>
#else
#  include <semaphore.h>
#endif // os
#include "../include/semaphore.h"

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
