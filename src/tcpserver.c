#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <errno.h>
#include <string.h>
#include "../include/simple/tcpserver.h"
#include "fd_impl.h"
#include "stream_impl.h"
#include "../include/simple/list.h"

#ifdef WIN32
#else
#  include <unistd.h>
#  define closesocket close
#endif // os

/** 对应一个 peer 对象 */
typedef struct client
{
	list_head head;

	int sock;
	void *opaque;	// 在 cb_open() 中，由调用者赋值 ..

	stream_t *stream;
	fd_t *fd;

} client;

typedef struct tcpserver_callbacks callbacks;

struct tcpserver_t
{
	int sock_listen;
	fd_t *s;

	callbacks cbs;

	list_head clients;	// 所有活动对象

	fd_set fdread, fdwrite, fdexp;
	int maxfd;
};

static client* _make_client(int sock, void *opaque)
{
	client *c = (client*)malloc(sizeof(client));
	list_init(&c->head);
	c->sock = sock;
	c->opaque = opaque;
	c->fd = simple_fd_open_from_socket(c->sock);
	c->stream = simple_stream_open(c->fd);
	c->fd->set_nonblock(c->fd, 1);	// 总是使用非阻塞 socket

	return c;
}

static int _release_client(client *c)
{
	int sock = c->sock;

	simple_stream_close(c->stream);
	simple_fd_close(c->fd);
	free(c);

	return sock;
}

static int _start_tcp_listen_sock(int port, const char *bindip)
{
	struct sockaddr_in sin;
	int enable = 1;

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		fprintf(stderr, "%s: socket create err, (%d) %s\n", __FUNCTION__, errno, strerror(errno));
		return sock;
	}

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(enable));

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = bindip ? inet_addr(bindip) : INADDR_ANY;
	if (bind(sock, (const struct sockaddr*)&sin, sizeof(sin)) == -1) {
		closesocket(sock);
		return -1;
	}

	if (listen(sock, SOMAXCONN) == -1) {
		closesocket(sock);
		return -1;
	}

	return sock;
}

/// 根据 clients 列表，更新 fds
static void _update_clients(tcpserver_t *ts)
{
	list_head *pos;
	
	ts->maxfd = ts->sock_listen;

	FD_ZERO(&ts->fdread);
	// FD_ZERO(&ts->fdwrite);
	// FD_ZERO(&ts->fwexp);

	FD_SET(ts->sock_listen, &ts->fdread);

	list_for_each(pos, &ts->clients) {
		client *c = (client*)pos;

		if (c->sock > ts->maxfd) ts->maxfd = c->sock;

		FD_SET(c->sock, &ts->fdread);

		//if (ts->cbs.cb_write)
		//	FD_SET(c->sock, &ts->fdwrite);
	}
}

tcpserver_t *simple_tcpserver_open(int port, const callbacks *cbs, const char *bindip)
{
	tcpserver_t *ts = (tcpserver_t*)malloc(sizeof(tcpserver_t));
	ts->sock_listen = -1;
	list_init(&ts->clients);
	memcpy(&ts->cbs, cbs, sizeof(callbacks));

	if ((ts->sock_listen = _start_tcp_listen_sock(port, bindip)) == -1) {
		free(ts);
		return 0;
	}

	fprintf(stderr, "%s: start server at %s:%d\n", __FUNCTION__, bindip ? bindip : "0.0.0.0", port);

	FD_ZERO(&ts->fdread);
	FD_ZERO(&ts->fdwrite);
	FD_ZERO(&ts->fdexp);

	FD_SET(ts->sock_listen, &ts->fdread);
	ts->maxfd = ts->sock_listen;

	ts->s = simple_fd_open_from_socket(ts->sock_listen);
	ts->s->set_nonblock(ts->s, 1);	// 使用非阻塞 sock

	return ts;
}

void simple_tcpserver_close(tcpserver_t *ts)
{
	// 释放所有 clients
	list_head *pos, *n;
	list_for_each_safe(pos, n, &ts->clients) {
		client *c = (client *)pos;
		list_del(pos);
		closesocket(c->sock);
		_release_client(c);
	}

	// 
	simple_fd_close(ts->s);
	closesocket(ts->sock_listen);
	free(ts);
}

void simple_tcpserver_runonce(tcpserver_t *ts)
{
	fd_set fdr = ts->fdread, fdw = ts->fdwrite, fde = ts->fdexp;
	struct timeval tv = { 0, 1000 * 300 };	/*  300 超时  */

	int rc = select(ts->maxfd, &fdr, &fdw, &fde, &tv);
	if (rc == 0) {
		// 超时 ..
	}
	else if (rc < 0) {
		fprintf(stderr, "%s: select err (%d) %s\n", __FUNCTION__, errno, strerror(errno));
	}
	else {
		// 检查所有 client sock
		list_head *pos, *n;
		int changed = 0;
		list_for_each_safe(pos, n, &ts->clients) {
			client *c = (client*)pos;

			if (FD_ISSET(c->sock, &fdr)) {
				// XXX： 检查是否为关闭？
				if (c->fd->is_eof(c->fd)) {
					if (ts->cbs.cb_close)
						ts->cbs.cb_close(c->opaque);

					list_del(pos);
					closesocket(c->sock);
					_release_client(c);

					changed = 1;
				}
				else {
					// 通知有数据可以读取
					if (ts->cbs.cb_read)
						ts->cbs.cb_read(c->opaque, c->stream);
				}
			}

			// FIXME: 可能太频繁了吧 :(
			if (FD_ISSET(c->sock, &fdw)) {
				if (ts->cbs.cb_write) {
					ts->cbs.cb_write(c->opaque, c->stream);
				}
			}
		}

		// 检查 listen sock
		if (FD_ISSET(ts->sock_listen, &fdr)) {
			struct sockaddr from;
			socklen_t fromlen = sizeof(from);
			int sock = accept(ts->sock_listen, &from, &fromlen);
			if (sock == -1) {
				fprintf(stderr, "%s: accept err (%d) %s\n", __FUNCTION__, errno, strerror(errno));
			}
			else {
				if (!ts->cbs.cb_open) {
					// 直接关闭即可
					closesocket(sock);
				}
				else {
					void *opaque;
					if (ts->cbs.cb_open(&opaque, &from, fromlen) < 0) {
						// 用户不关心该链接，直接释放
						closesocket(sock);
					}
					else {
						// TODO: 检查是否支持更多的 sock ?

						// 构造 client
						client *c = _make_client(sock, opaque);
						list_add(&c->head, &ts->clients);

						changed = 1;
					}
				}
			}
		}

		if (changed) {
			_update_clients(ts);
		}
	}
}

void simple_tcpserver_run(tcpserver_t *ts, int *quit)
{
	while (!(*quit)) {
		simple_tcpserver_runonce(ts);
	}
}
