#ifndef _simple_mutex__hh
#define _simple_mutex__hh

typedef struct mutex_t mutex_t;

mutex_t *simple_mutex_create();
void simple_mutex_destroy(mutex_t *m);

void simple_mutex_lock(mutex_t *m);
void simple_mutex_unlock(mutex_t *m);

#endif // mutex.h
