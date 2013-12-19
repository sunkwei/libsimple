#ifndef simple_httpserver__hh
#define simple_httpserver__hh

#include "httpparser.h"
#include "stream.h"

typedef struct httpserver_t httpserver_t;

typedef struct httpserver_callbacks
{
	/** ���ڴ��� request
		
		@param req: �������req msg����Ҫ����Ƿ�״̬������״̬Ϊ BODY �� COMPLETE����� BODY������ʹ�� simple_httpserver_read_body(..) ��ȡ body
		@param res: д��������� body ̫�� ( > 64k ?)��Ӧ��ʹ simple_httpserver_write_body(...)
	 */
	int (*cb_request)(httpserver_t *hs, void *opaque, const HttpMessage *req, HttpMessage *res);

} httpserver_callbacks;

httpserver_t *simple_httpserver_open(int port, const httpserver_callbacks, const char *bindip);
void simple_httpserver_close(httpserver_t *hs);

void simple_httpserver_runonce(httpserver_t *hs);
void simple_httpserver_run(httpserver_t *hs, int *quit);

/** ������ cb_request() �е��� */
int simple_httpserver_read_body(httpserver_t *hs, void *opaque, void *buf, int len);
int simple_httpserver_write_body(httpserver_t *hs, void *opaque, const void *buf, int len);

#endif // httpserver.h
