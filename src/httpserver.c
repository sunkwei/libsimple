#include <stdio.h>
#include <stdlib.h>
#include "../include/simple/httpserver.h"
#include "../include/simple/tcpserver.h"
#include "stream_impl.h"
#include "fd_impl.h"

struct httpserver_t
{
	tcpserver_t *ts;

	httpserver_callbacks cbs;
};

static int _ts_accept(tcpserver_t *ts, void **opaque, const struct sockaddr *addrfrom, socklen_t addrlen)
{
	return -1;
}

static void _ts_read(tcpserver_t *ts, void *opaque, stream_t *s)
{
}

static void _ts_write(tcpserver_t *ts, void *opaque, stream_t *s)
{
}

static void _ts_close(tcpserver_t *ts, void *opaque)
{
}

httpserver_t *simple_httpserver_open(int port, const httpserver_callbacks *cbs, const char *bind)
{
	httpserver_t *hs = (httpserver_t*)malloc(sizeof(httpserver_t));
	tcpserver_callbacks tscbs = { _ts_accept, _ts_read, _ts_write, _ts_close };
	hs->ts = simple_tcpserver_open(port, &tscbs, bind);
	if (!hs->ts) {
		free(hs);
		return 0;
	}

	hs->cbs = *cbs;

	return hs;
}

void simple_httpserver_close(httpserver_t *hs)
{
}

void simple_httpserver_runonce(httpserver_t *hs)
{
}

void simple_httpserver_run(httpserver_t *hs, int *quit)
{
}

int simple_httpserver_read_body(httpserver_t *hs, void *opaque, void *buf, int len)
{
	return -1;
}

int simple_httpserver_write_body(httpserver_t *hs, void *opaque, const void *buf, int len)
{
	return -1;
}
