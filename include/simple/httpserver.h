#ifndef simple_httpserver__hh
#define simple_httpserver__hh

#include "httpparser.h"
#include "stream.h"

typedef struct httpserver_t httpserver_t;

typedef struct httpserver_callbacks
{
	/** 用于处理 request
		
		@param req: 解析后的req msg，需要检查是否状态，可能状态为 BODY 或 COMPLETE，如果 BODY，可以使用 simple_httpserver_read_body(..) 读取 body
		@param res: 写入结果，如果 body 太大 ( > 64k ?)，应该使 simple_httpserver_write_body(...)
	 */
	int (*cb_request)(httpserver_t *hs, void *opaque, const HttpMessage *req, HttpMessage *res);

} httpserver_callbacks;

httpserver_t *simple_httpserver_open(int port, const httpserver_callbacks, const char *bindip);
void simple_httpserver_close(httpserver_t *hs);

void simple_httpserver_runonce(httpserver_t *hs);
void simple_httpserver_run(httpserver_t *hs, int *quit);

/** 必须在 cb_request() 中调用 */
int simple_httpserver_read_body(httpserver_t *hs, void *opaque, void *buf, int len);
int simple_httpserver_write_body(httpserver_t *hs, void *opaque, const void *buf, int len);

#endif // httpserver.h
