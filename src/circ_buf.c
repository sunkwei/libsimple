#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include "../include/simple/circ_buf.h"
#include "../include/simple/mutex.h"

typedef struct circ_buf_t
{
	unsigned char *buf;
	int head, tail;
	int size;

	mutex_t *mutex;
} circ_buf_impl;

circ_buf_t *simple_circ_buf_create(int size)
{
	circ_buf_impl *c = (circ_buf_impl*)malloc(sizeof(circ_buf_impl));
	c->buf = (unsigned char*)malloc(size+1);
	c->head = 0;
	c->tail = 0;
	c->size = size;
	c->mutex = simple_mutex_create();

	return (circ_buf_t*)c;
}

void simple_circ_buf_destroy(circ_buf_t *c)
{
	free(c->buf);
	simple_mutex_destroy(c->mutex);
	free(c);
}

static int _cnt(int head, int tail, int size)
{
	return ((head + size) - tail) % size;
}

static int _space(int head, int tail, int size)
{
	return _cnt(tail, head+1, size);
}

int simple_circ_buf_get_data_cnt(circ_buf_t *c)
{
	return _cnt(c->head, c->tail, c->size);
}

int simple_circ_buf_get_space_cnt(circ_buf_t *c)
{
	return _space(c->head, c->tail, ((circ_buf_impl*)c)->size);
}

static int _circ_cnt_to_end(int head, int tail, int size)
{
	int end = size - tail;
	int n = (head + end) % (size);
	if (n < end)
		return n;
	else
		return end;
}

static int _circ_space_to_end(int head, int tail, int size)
{
	int end = size - 1 - head;
	int n = (end + tail) % (size);
	if (n <= end)
		return n;
	else
		return end+1;
}

int simple_circ_buf_read(circ_buf_t *c, void *buf, int size)
{
	int n;

	simple_mutex_lock(c->mutex);
	n = _circ_cnt_to_end(c->head, c->tail, c->size);

	if (size <= n) {
		// 一次即可
		memcpy(buf, c->buf+c->tail, size);
		c->tail += size;
		c->tail %= c->size;

		simple_mutex_unlock(c->mutex);
		return size;
	}
	else {
		int total = n;
		unsigned char *to = (unsigned char*)buf;
		// 第一次
		memcpy(to, c->buf+c->tail, n);
		c->tail += n;
		c->tail %= c->size;

		// 第二次
		to += n;
		size -= n;

		n = _circ_cnt_to_end(c->head, c->tail, c->size);
		n = size >= n ? n : size;

		memcpy(to, c->buf+c->tail, n);
		c->tail += n;
		c->tail %= c->size;

		simple_mutex_unlock(c->mutex);
		return total+n;
	}
}

int simple_circ_buf_write(circ_buf_t *c, const void *data, int size)
{
	int n;
	
	simple_mutex_lock(c->mutex);
	n = _circ_space_to_end(c->head, c->tail, c->size);

	if (size <= n) {
		// 一次即可
		memcpy(c->buf+c->head, data, size);
		c->head += size;
		c->head %= c->size;

		simple_mutex_unlock(c->mutex);
		return size;
	}
	else {
		int total = n;
		unsigned char *from = (unsigned char *)data;

		// 第一次
		memcpy(c->buf+c->head, from, n);
		c->head += n;
		c->head %= c->size;

		// 第二次
		from += n;
		size -= n;

		n = _circ_space_to_end(c->head, c->tail, c->size);
		n = size >= n ? n : size;

		memcpy(c->buf+c->head, from, n);
		c->head += n;
		c->head %= c->size;

		simple_mutex_unlock(c->mutex);
		return total+n;
	}
}
