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
#include "../include/simple/mutex.h"

#ifdef WIN32
#else
#  include <unistd.h>
#  define closesocket close
#endif // os

typedef enum select_flag
{
	SF_READ = 1,
	SF_WRITE = 2,
	SF_EXCEPT = 4,
} select_flags;

/** 对应一个 peer 对象 */
typedef struct client
{
	list_head head;

	int sock;
	void *opaque;	// 在 cb_open() 中，由调用者赋值 ..

	stream_t *stream;
	fd_t *fd;

	int select_flags;
} client;

typedef struct tcpserver_callbacks callbacks;

struct tcpserver_t
{
	int sock_listen;
	fd_t *s;

	callbacks cbs;

	list_head clients;	// 所有活动对象.
	mutex_t *lock;		// 锁定.

	fd_set fdread, fdwrite, fdexp;
	int maxfd;

	int need_update;	// simple_tcpserver_enable_write_callback()
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
	c->select_flags = SF_READ;		// 缺省仅仅使用 read 通知.

	return c;
}

static client* _find_client_from_stream(tcpserver_t *ts, stream_t *s)
{
	client *c = 0;
	list_head *pos;

	simple_mutex_lock(ts->lock);
	list_for_each(pos, &ts->clients) {
		client *cc = (client*)pos;

		if (s == cc->stream) {
			c = cc;
			break;
		}
	}
	simple_mutex_unlock(ts->lock);

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
	FD_ZERO(&ts->fdwrite);
	FD_ZERO(&ts->fdexp);

	// listen sock
	FD_SET(ts->sock_listen, &ts->fdread);

	// all client socks
	simple_mutex_lock(ts->lock);

	list_for_each(pos, &ts->clients) {
		client *c = (client*)pos;

		if (c->sock > ts->maxfd) ts->maxfd = c->sock;

		if (c->select_flags & SF_READ)
			FD_SET(c->sock, &ts->fdread);

		if (ts->cbs.cb_write && c->select_flags & SF_WRITE)
			FD_SET(c->sock, &ts->fdwrite);

//		if (c->select_flags & SF_EXCEPT)
//			FD_SET(c->sock, &ts->fdexp);
	}

	simple_mutex_unlock(ts->lock);
}

int simple_tcpserver_enable_write_callback(tcpserver_t *ts, stream_t *s, int enable)
{
	client *c = 0;

	if (!ts->cbs.cb_write)
		return -1;	// 不支持.

	c = _find_client_from_stream(ts, s);
	if (c) {
		if (enable)
			c->select_flags |= SF_WRITE;
		else
			c->select_flags &= ~SF_WRITE;

		ts->need_update = 1;	// 需要更新..
	}

	return 0;
}

tcpserver_t *simple_tcpserver_open(int port, const callbacks *cbs, const char *bindip)
{
	tcpserver_t *ts = (tcpserver_t*)malloc(sizeof(tcpserver_t));
	ts->sock_listen = -1;
	list_init(&ts->clients);
	memcpy(&ts->cbs, cbs, sizeof(callbacks));
	ts->need_update = 0;

	if ((ts->sock_listen = _start_tcp_listen_sock(port, bindip)) == -1) {
		free(ts);
		return 0;
	}

	ts->lock = simple_mutex_create();

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
	simple_mutex_destroy(ts->lock);
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
		int count = 0;
		
		simple_mutex_lock(ts->lock);

		list_for_each_safe(pos, n, &ts->clients) {
			client *c = (client*)pos;

			if (FD_ISSET(c->sock, &fdr)) {
				// XXX： 检查是否为关闭？
				if (c->fd->is_eof(c->fd)) {
					if (ts->cbs.cb_close)
						ts->cbs.cb_close(ts, c->opaque);

					list_del(pos);
					closesocket(c->sock);
					_release_client(c);

					changed = 1;
				}
				else {
					// 通知有数据可以读取
					if (ts->cbs.cb_read)
						ts->cbs.cb_read(ts, c->opaque, c->stream);
				}
			}

			// 可能太频繁了吧 :(
			if (FD_ISSET(c->sock, &fdw)) {
				if (ts->cbs.cb_write) {
					ts->cbs.cb_write(ts, c->opaque, c->stream);
				}
			}

			count++;
		}

		simple_mutex_unlock(ts->lock);

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
					// 直接关闭即可.
					closesocket(sock);
				}
				else {
					void *opaque;
					if (ts->cbs.cb_open(ts, &opaque, &from, fromlen) < 0) {
						// 用户不关心该链接，直接释放.
						closesocket(sock);
					}
					else {
						// TODO: 检查是否支持更多的 sock ?
						if (count + 1 >= FD_SETSIZE) {
							// 太多了 .
							closesocket(sock);
						}
						else {
							// 构造 client
							client *c = _make_client(sock, opaque);
							list_add(&c->head, &ts->clients);

							changed = 1;
						}
					}
				}
			}
		}

		if (changed) {
			_update_clients(ts);
		}
	}

	if (ts->need_update) {
		_update_clients(ts);
		ts->need_update = 0;
	}
}

void simple_tcpserver_run(tcpserver_t *ts, int *quit)
{
	while (!(*quit)) {
		simple_tcpserver_runonce(ts);
	}
}
