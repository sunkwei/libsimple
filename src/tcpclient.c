#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#ifdef WIN32
#  include <WinSock2.h>
#  include <WS2tcpip.h>
#else
#  include <sys/socket.h>
#  include <sys/types.h>
#  include <netdb.h>
#  include <errno.h>
#  include <unistd.h>
#  define closesocket close
#endif // os
#include "../include/simple/tcpclient.h"
#include "fd_impl.h"

struct tcpclient_t
{
	int sock;
	fd_t *fd;
};

static int _connect(int sock, struct sockaddr *addr, int len, int timeout)
{
	int rc, wait = 0;
	fd_set fds;
	fd_t *fd = simple_fd_open_from_socket(sock);
	fd->set_nonblock(fd, 1);	// 注意：总是使用非阻塞 io 模式

	rc = connect(sock, addr, len);
	if (rc == -1) {
#ifdef WIN32
		if (WSAGetLastError() == WSAEWOULDBLOCK) {
			wait = 1;
		}
#else
		if (errno == EINPROGRESS) {
			wait = 1;
		}
#endif // os
	}

	if (wait) {
		struct timeval tv = { timeout / 1000, (timeout % 1000) * 1000 };
		FD_ZERO(&fds);
		FD_SET(sock, &fds);

		rc = select(sock+1, 0, &fds, 0, &tv);
		if (rc <= 0)
			rc = -1;
		else {
			if (FD_ISSET(sock, &fds))
				rc = 0;
			else
				rc = -1;
		}
	}
	simple_fd_close(fd);

	return rc;
}

static int _try_connect(const char *server, int port, int timeout)
{
	struct addrinfo hints, *addr, *p;
	char serv_name[16];
	int sock = -1;

	sprintf(serv_name, "%d", port);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	//hints.ai_protocol = IPPROTO_TCP;

	if (getaddrinfo(server, serv_name, &hints, &addr) == -1) {
		return -1;
	}

	for (p = addr; p; p = p->ai_next) {
		sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sock != -1) {
			if (_connect(sock, p->ai_addr, p->ai_addrlen, timeout) < 0) {
				closesocket(sock);
				sock = -1;
			}
			else {
				// 成功
				break;
			}
		}
	}

	freeaddrinfo(addr);

	return sock;
}

tcpclient_t *simple_tcpclient_open(const char *server, int port, int timeout)
{
	tcpclient_t *tc = (tcpclient_t*)malloc(sizeof(tcpclient_t));
	tc->sock = _try_connect(server, port, timeout);
	if (tc->sock == -1) {
		free(tc);
		return 0;
	}
	tc->fd = simple_fd_open_from_socket(tc->sock);

	return tc;
}

void simple_tcpclient_close(tcpclient_t *tc)
{
	simple_fd_close(tc->fd);
	closesocket(tc->sock);
	free(tc);
}

int simple_tcpclient_sendn(tcpclient_t *tc, const void *data, int len, int *sent)
{
	return simple_tcpclient_sendnt(tc, data, len, sent, 0x7fffffff);
}

int simple_tcpclient_send(tcpclient_t *tc, const void *data, int len, int *sent)
{
	return simple_tcpclient_sendt(tc, data, len, sent, 0x7fffffff);
}

int simple_tcpclient_sendt(tcpclient_t *tc, const void *data, int len, int *sent, int timeout)
{
	struct timeval tv = { timeout / 1000, (timeout % 1000) * 1000 };
	fd_set fd;
	int rc;

	FD_ZERO(&fd);
	FD_SET(tc->sock, &fd);

	*sent = 0;

retry:
	rc = select(tc->sock+1, 0, &fd, 0, &tv);
	if (rc == 0) {
		return -2;	// 超时
	}
	else if (rc == -1) {
		return -1;	// 错误
	}
	else {
		rc = send(tc->sock, (const char*)data, len, 0);
		if (rc >= 0) {
			*sent = rc;
			return rc;
		}
		else {
#ifdef WIN32
			if (WSAGetLastError() == WSAEWOULDBLOCK) {
				goto retry;
			}
#else
			if (errno == EAGAIN) {
				goto retry;
			}
#endif // os
			else {
				return -1;
			}
		}
	}
}

int simple_tcpclient_recv(tcpclient_t *tc, void *buf, int len, int *recved)
{
	return simple_tcpclient_recvt(tc, buf, len, recved, 0x7fffffff);
}

int simple_tcpclient_recvn(tcpclient_t *tc, void *buf, int len, int *recved)
{
	return simple_tcpclient_recvnt(tc, buf, len, recved, 0x7fffffff);
}

int simple_tcpclient_recvt(tcpclient_t *tc, void *buf, int len, int *recved, int timeout)
{
	struct timeval tv = { timeout / 1000, (timeout % 1000) * 1000 };
	fd_set fd;
	int rc;

	FD_ZERO(&fd);
	FD_SET(tc->sock, &fd);

	*recved = 0;

retry:
	rc = select(tc->sock+1, &fd, 0, 0, &tv);
	if (rc == 0) {
		return -2;	// 超时
	}
	else if (rc == -1) {
		return -1;	// 错误
	}
	else {
		rc = recv(tc->sock, (char*)buf, len, 0);
		if (rc >= 0) {
			*recved = rc;
			return rc;
		}
		else {
#ifdef WIN32
			if (WSAGetLastError() == WSAEWOULDBLOCK) {
				goto retry;
			}
#else
			if (errno == EAGAIN) {
				goto retry;
			}
#endif // os
			else {
				return -1;
			}
		}
	}
}

int simple_tcpclient_sendnt(tcpclient_t *tc, const void *data, int len, int *sent, int timeout)
{
	// FIXME：超时计算不对 .

	const char *next = (const char*)data;
	int rest = len;

	*sent = 0;

	while (rest > 0) {
		int bytes;
		int rc = simple_tcpclient_sendt(tc, next, rest, &bytes, timeout);
		if (rc > 0) {
			rest -= bytes;
			next += bytes;

			*sent += bytes;
		}
		else {
			return rc;
		}
	}

	return len;
}

int simple_tcpclient_recvnt(tcpclient_t *tc, void *buf, int len, int *recved, int timeout)
{
	// FIXME: 超时计算不对

	char *next = (char *)buf;
	int rest = len;

	*recved = 0;

	while (rest > 0) {
		int bytes;
		int rc = simple_tcpclient_recvt(tc, next, rest, &bytes, timeout);
		if (rc > 0) {
			rest -= bytes;
			next += bytes;

			*recved += bytes;
		}
		else {
			return rc;
		}
	}

	return len;
}
