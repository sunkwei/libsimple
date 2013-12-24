#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#ifdef WIN32
#  include <winsock2.h>
#else
#  include <errno.h>
#  include <unistd.h>
#  include <sys/socket.h>
#  include <sys/types.h>
#  include <arpa/inet.h>

#  define closesocket close
#endif // os
#include "../include/simple/httpparser.h"
#include "../include/simple/url.h"
#include "client.h"
#include "httprequest.h"

static int lasterr()
{
#ifdef WIN32
	return WSAGetLastError();
#else
	return errno;
#endif // os
}

static void handle(task_t *t, void *opaque);

client *build_client(int sock)
{
	client *c = (client*)malloc(sizeof(client));
	c->sock = sock;
	c->handle = handle;

	return c;
}

static HttpMessage *make_request(client *c)
{
	/** 从 c->sock recv()，并且使用 http parser 解析，直到收到完整 http msg
	    如果失败，则返回 0
	 */
	int used;
	char ch;
	HttpMessage *req = 0;
	HttpParserState st;

	do {
		int rc = recv(c->sock, &ch, 1, 0);
		if (rc == 0) {
			fprintf(stdout, "INFO: recv sock(%d) closed\n", c->sock);
			break;
		}
		else if (rc == -1) {
			fprintf(stderr, "ERR: recv sock(%d) err, code=%d, %s\n", c->sock, lasterr(), strerror(errno));
			break;
		}

		req = httpc_parser_parse(req, &ch, 1, &used);
		if (!req)
			break;

		st = httpc_Message_state(req);
	} while (st != HTTP_COMPLETE && st != HTTP_BODY);

	if (req) {
		st = httpc_Message_state(req);
		if (st == HTTP_COMPLETE || st == HTTP_BODY) {
			return req;
		}

		httpc_Message_release(req);
		req = 0;
	}

	return req;
}

static HttpMessage *make_default_response()
{
	HttpMessage *res = httpc_Message_create();
	httpc_Message_setStartLine(res, "HTTP/1.0", "200", "OK");
	httpc_Message_setValue(res, "Server", "test_http_srv 1.0");
	httpc_Message_setValue(res, "Connection", "Close");	// 总是主动关闭.

	return res;
}

/** 在这里处理一次完整的 http request，将结果填充到 res 中，返回 < 0 失败.
 */
static int handle_http(client *c, const HttpMessage *req, HttpMessage *res)
{
	int rc;
	url_t *url = simple_url_parse(req->StartLine.p2);	// start line p2 为 url
	if (!url) {
		fprintf(stderr, "ERR: sock(%d) parse url %s err\n", c->sock, req->StartLine.p2);
		return -1;
	}

	rc = http_request(c, url, req, res);
	simple_url_release(url);

	return rc;
}

/** 将 res encode，发送.
 */
static int send_res(client *c, HttpMessage *res)
{
	int len = httpc_Message_get_encode_length(res, 1);
	char *buf = (char*)malloc(len+1);
	char *p = buf;
	httpc_Message_encode(res, buf, 1);

	while (len > 0) {
		int rc = send(c->sock, p, len, 0);
		if (rc > 0) {
			len -= rc;
			p += rc;
		}
		else {
			fprintf(stderr, "ERR: send err, code=%d\n", lasterr());
			free(buf);
			return -1;
		}
	}

	free(buf);
	return 0;
}

static void handle(task_t *t, void *opaque)
{
	/** 这里处理一次完整的 http 链接，当这个函数返回时，就中断该链接了. 
	 */
	client *c = (client*)opaque;
	int rc;

	do {
		HttpMessage *req = make_request(c), *res = 0;
		if (!req) {
			fprintf(stderr, "INFO: sock(%d) recv err | connection closed\n", c->sock);
			break;
		}

		res = make_default_response();
		
		rc = handle_http(c, req, res);
		if (rc < 0) {
			fprintf(stderr, "ERR: sock(%d) handle http err\n", c->sock);

			httpc_Message_release(req);
			httpc_Message_release(res);

			break;
		}
		else if (rc > 0) {
			// 此时说明在 handle_http 中，已经发送 res 了！！！.
			// 一般是发送文件了 ,....
		}
		else if (send_res(c, res) < 0) {
			fprintf(stderr, "ERR: sock(%d) send response err\n", c->sock);
			httpc_Message_release(req);
			httpc_Message_release(res);

			break;
		}

		httpc_Message_release(req);
		httpc_Message_release(res);
	} while (0);	// 对于 http pipe 来说，应该保持连接，这个简单，每次链接仅仅处理一个 http request,
					// res 中，总是包含 Connection: close.

	simple_task_destroy(t);
	fprintf(stdout, "INFO: connection (%d) terminated!\n", c->sock);

	closesocket(c->sock);
}
