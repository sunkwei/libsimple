#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
#  include <WinSock2.h>
#endif // os
#include "../include/simple/tcpclient.h"

int main()
{
	tcpclient_t *tc;
#ifdef WIN32
	WSADATA data;
	WSAStartup(0x202, &data);
#endif // os

	tc = simple_tcpclient_open("172.16.1.10", 80, 5000);
	if (tc) {
		const char *req = 
			"GET / HTTP/1.0\r\n"
			"Connection: close\r\n"
			"\r\n";

		int bytes;
		int rc = simple_tcpclient_sendn(tc, req, strlen(req), &bytes);
		if (rc == strlen(req)) {
			// Ω” ’
			char c;
			do {
				rc = simple_tcpclient_recvn(tc, &c, 1, &bytes);
				if (rc == 1) {
					fprintf(stdout, "%c", c);
				}
				else if (rc == 0) {
					fprintf(stdout, "net closed\n");
					break;
				}
				else {
					fprintf(stderr, "net err, rc=%d\n", rc);
					break;
				}
			} while (1);
		}

		simple_tcpclient_close(tc);
	}

	return 0;
}
