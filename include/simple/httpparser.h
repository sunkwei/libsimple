#ifndef _httpclient_hh_
#define _httpclient_hh_

#include "list.h"

typedef enum HttpParserState
{
	HTTP_UNKNOWN,
	HTTP_STARTLINE,
	HTTP_HEADERS,
	HTTP_BODY,
	HTTP_COMPLETE,
	HTTP_CR,
	HTTP_CRLF,
	HTTP_LSW,
} HttpParserState;

// header
struct HttpHeader
{
	list_head head;	// 用于支持 list

	char *key;
	char *value;
};
typedef struct HttpHeader HttpHeader;

/// HTTP Message 
struct HttpMessage
{
	// Start line
	struct {
		/** 对于 request: GET /path/cmd?aa=x HTTP/1.1 */
		/** 对于 response: HTTP/1.1  200  OK */
		char *p1;
		char *p2;
		char *p3;
	} StartLine;

	/** 使用 list_for_each  访问 */
	list_head headers;
	int header_cnt;

	// body
	char *body;
	int bodylen;

	// private data
	void *pri_data;
};
typedef struct HttpMessage HttpMessage;

HttpMessage *httpc_Message_create();
void httpc_Message_release(HttpMessage *msg);

void httpc_Message_setStartLine(HttpMessage *msg, const char *p1, const char *p2, const char *p3);
int httpc_Message_setValue(HttpMessage *msg, const char *key, const char *value);
int httpc_Message_getValue(HttpMessage *msg, const char *key, const char **value);
int httpc_Message_delValue(HttpMessage *msg, const char *key);
int httpc_Message_appendBody(HttpMessage *msg, const char *body, int len);
void httpc_Message_clearBody(HttpMessage *msg);
int httpc_Message_getBody(HttpMessage *msg, const char **body);

/** 返回需要 encode 的字节长度 */
int httpc_Message_get_encode_length(HttpMessage *msg, int with_body);
/** buf 长度至少 httpc_Message_get_encode_length() */
void httpc_Message_encode(HttpMessage *msg, char *buf, int with_body);

/** 返回解析状态 */
HttpParserState httpc_Message_state(HttpMessage *msg);

HttpMessage *httpc_parser_parse(HttpMessage *saved, const char *data, int len, int *used);

#endif // httpclient.h
