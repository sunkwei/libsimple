/** 一个典型的多线程sock server，
    启动一个线程池；
    主线程阻塞 accept，收到链接后，将client sock放到线程池中；
    工作线程阻塞 recv, 收到完整 http request 后，执行具体的处理;
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "../include/simple/threadpool.h"

#ifdef WIN32
#  include <winsock2.h>
typedef int socklen_t;
#else
#  include <errno.h>
#  include <unistd.h>
#  include <sys/socket.h>
#  include <sys/types.h>
#  include <arpa/inet.h>

#  define closesocket close
#endif // os

#include "client.h"

static int lasterr()
{
#ifdef WIN32
	return WSAGetLastError();
#else
	return errno;
#endif // os
}

int main(int argc, char **argv)
{
	int sock = -1;	// listen sock
	thread_pool_t *tp = 0;	// thread pool
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	int port = 8100;	// port
	const char *local = "0.0.0.0";
	int quit = 0;	// 结束.  

#ifdef WIN32
	WSADATA data;
	WSAStartup(0x202, &data);
#endif // os

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		fprintf(stderr, "ERR: create sock err, code=%d\n", lasterr());
		return -1;
	}

	// bind
	memset(&addr, 0, addrlen);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(local);
	if (bind(sock, (const struct sockaddr*)&addr, addrlen) == -1) {
		fprintf(stderr, "ERR: bind sock port %d err, code=%d\n", port, lasterr());
		closesocket(sock);
		return -1;
	}

	// listen
	if (listen(sock, SOMAXCONN) == -1) {
		fprintf(stderr, "ERR: listen sock err, code=%d\n", lasterr());
		closesocket(sock);
		return -1;
	}

	fprintf(stdout, "INFO: en, the http server listen on %s:%d\n",
			local, port);

	// thread pool
	tp = simple_thread_pool_create(8);

	// accept ... loop
	while (!quit) {
		int sock_client = -1;
		struct sockaddr_in from;
		socklen_t fromlen = sizeof(from);

		sock_client = accept(sock, (struct sockaddr *)&from, &fromlen);
		if (sock_client == -1) {
			fprintf(stderr, "ERR: accept sock err, code=%d\n", lasterr());
			closesocket(sock);
			return -1;
		}
		else {
			// accept success, and build a client into thread pool
			client *c = build_client(sock_client);
			task_t *t = simple_task_create(c->handle, c);

			fprintf(stdout, "INFO: connect from %s:%d\n",
					inet_ntoa(from.sin_addr), ntohs(from.sin_port));

			simple_thread_pool_add_task(tp, t);
		}
	}

	closesocket(sock);
	simple_thread_pool_destroy(tp);

	return 0;
}

