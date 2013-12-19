#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#ifdef WIN32
#  include <WinSock2.h>
#else
#endif // os
#include "fd_impl.h"
#include "stream_impl.h"

stream_t *simple_stream_open(fd_t *fd)
{
	stream_t *s = (stream_t*)malloc(sizeof(stream_t));
	s->fd = fd;
	return s;
}

void simple_stream_close(stream_t *s)
{
	free(s);
}

int simple_stream_read(stream_t *s, void *buf, int bufsize, int *bytes)
{
	int rc = s->fd->read(s->fd, buf, bufsize);
	if (rc > 0) {
		*bytes = rc;
		return 0;
	}
	else {
		return s->fd->lasterr(s->fd);
	}
}

int simple_stream_write(stream_t *s, const void *data, int datalen, int *bytes)
{
	int rc = s->fd->write(s->fd, data, datalen);
	if (rc > 0) {
		*bytes = rc;
		return 0;
	}
	else {
		return s->fd->lasterr(s->fd);
	}
}
