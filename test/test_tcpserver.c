#include <stdio.h>
#include <stdlib.h>
#include "../include/simple/tcpserver.h"

static int cb_open(tcpserver_t *ts, void **opaque, const struct sockaddr *from, socklen_t len)
{
	if (from->sa_family == AF_INET) {
		struct sockaddr_in *in = (struct sockaddr_in*)from;

		fprintf(stderr, "%s: connection from %s:%d\n", __FUNCTION__, inet_ntoa(in->sin_addr), ntohs(in->sin_port));
	}

	return 0;
}

static void cb_read(tcpserver_t *ts, void *opaque, stream_t *s)
{
	char buf[16];
	int rc = simple_stream_read(s, buf, sizeof(buf)-1);
	if (rc > 0) {
		buf[rc] = 0;
		fprintf(stderr, "%s: recv: %s\n", __FUNCTION__, buf);

		simple_stream_write(s, buf, rc);
	}
	else {
		fprintf(stderr, "%s: read err?\n", __FUNCTION__);
	}
}

static void cb_close(tcpserver_t *ts, void *opaque)
{
	fprintf(stderr, "%s: called!\n", __FUNCTION__);
}

static int _quit = 0;

#ifdef WIN32
static int WINAPI ctrl_c_handle(DWORD code)
{
	_quit = 1;
	return 0;
}
#else
#endif // os

int main()
{
	tcpserver_t *ts;
	tcpserver_callbacks cbs = { cb_open, cb_read, 0, cb_close };

#ifdef WIN32
	WSADATA data;
	WSAStartup(0x202, &data);

	SetConsoleCtrlHandler(ctrl_c_handle, 1);
#endif // os


	ts = simple_tcpserver_open(5000, &cbs, 0);

	if (ts) {
		simple_tcpserver_run(ts, &_quit);
		simple_tcpserver_close(ts);
	}
}
