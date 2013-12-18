#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/simple/httpparser.h"

#ifdef WIN32
#define strcasecmp stricmp
#define snprintf _snprintf
#endif // win32

#define safefree(x) {	\
	if (x) free(x);	\
	x = 0;		\
}

HttpMessage *httpc_Message_create ()
{
	HttpMessage *msg = (HttpMessage *)malloc(sizeof(HttpMessage));

	msg->StartLine.p1 = 0;
	msg->StartLine.p2 = 0;
	msg->StartLine.p3 = 0;

	msg->pri_data = 0;

	msg->headers = 0;
	msg->header_cnt = 0;

	msg->body = 0;
	msg->bodylen = 0;

	return msg;
}

extern void httpc_internal_releaseParserData (void *pridata);

void httpc_Message_release (HttpMessage *msg)
{
	int i;
	if (msg) {
		safefree(msg->StartLine.p1);
		safefree(msg->StartLine.p2);
		safefree(msg->StartLine.p3);

		if (msg->headers) {
			for (i = 0; i < msg->header_cnt; i++) {
				safefree(msg->headers[i].key);
				safefree(msg->headers[i].value);
			}
			safefree(msg->headers);
		}
		msg->header_cnt = 0;

		safefree(msg->body);
		msg->bodylen = 0;

		httpc_internal_releaseParserData(msg->pri_data);
		msg->pri_data = 0;
	}
	free(msg);
}

void httpc_Message_setStartLine (HttpMessage *msg,
		const char *p1, const char *p2, const char *p3)
{
	if (msg && p1 && p2 && p3) {
		safefree(msg->StartLine.p1);
		msg->StartLine.p1 = strdup(p1);

		safefree(msg->StartLine.p2);
		msg->StartLine.p2 = strdup(p2);

		safefree(msg->StartLine.p3);
		msg->StartLine.p3 = strdup(p3);
	}
}

int httpc_Message_setValue (HttpMessage *msg,
		const char *key, const char *value)
{
	int i;
	if (!msg || !key || !value) return -1;
	for (i = 0; i < msg->header_cnt; i++) {
		if (!strcasecmp(key, msg->headers[i].key)) {
			safefree(msg->headers[i].value);
			msg->headers[i].value = strdup(value);

			return 1;
		}
	}

	msg->headers = (HttpHeader *)realloc(msg->headers,
			(msg->header_cnt+1) * sizeof(HttpHeader));
	msg->headers[msg->header_cnt].key = strdup(key);
	msg->headers[msg->header_cnt].value = strdup(value);

	msg->header_cnt++;

	return 0;
}

int httpc_Message_getValue (HttpMessage *msg,
		const char *key, const char **value)
{
	int i;
	if (!msg || !key) return -1;
	for (i = 0; i < msg->header_cnt; i++) {
		if (!strcasecmp(key, msg->headers[i].key)) {
			*value = msg->headers[i].value;
			return 1;
		}
	}

	return 0;
}

int httpc_Message_delValue (HttpMessage *msg,
		const char *key)
{
	// TODO: 优化的处理管理内存
	assert(0);
	return -1;
}

int httpc_Message_appendBody (HttpMessage *msg,
		const char *data, int len)
{
	char tmp[16];
	if (!msg) return -1;
	if (len == 0) return msg->bodylen;
	msg->body = (char*)realloc(msg->body, msg->bodylen+len);
	memcpy(&msg->body[msg->bodylen], data, len);
	msg->bodylen += len;

	snprintf(tmp, sizeof(tmp), "%u", msg->bodylen);
	httpc_Message_setValue(msg, "Content-Length", tmp);

	return msg->bodylen;
}

void httpc_Message_clearBody (HttpMessage *msg)
{
	if (msg) {
		safefree(msg->body);
		msg->bodylen = 0;
	}
}

int httpc_Message_getBody (HttpMessage *msg, const char **data)
{
	if (!msg) return -1;
	*data = msg->body;
	return msg->bodylen;
}

int httpc_Message_get_encode_length (HttpMessage *msg)
{
	int len = 0, i;
	if (!msg || !msg->StartLine.p1 || !msg->StartLine.p2 || !msg->StartLine.p3) return -1;

	len += strlen(msg->StartLine.p1) + 1; // ' '
	len += strlen(msg->StartLine.p2) + 1; // ' '
	len += strlen(msg->StartLine.p3);
	len += 2;	// \r\n

	for (i = 0; i < msg->header_cnt; i++) {
		len += strlen(msg->headers[i].key) + 2; // ": "
		len += strlen(msg->headers[i].value);
		len += 2;	// \r\n
	}

	len += 2;	// \r\n
	len += msg->bodylen;

	return len;
}

void httpc_Message_encode (HttpMessage *msg, char *buf)
{
	int i;
	if (msg) {
		char *p = buf;

		strcpy(p, msg->StartLine.p1);
		p += strlen(msg->StartLine.p1);
		strcpy(p, " ");
		p += 1;
		strcpy(p, msg->StartLine.p2);
		p += strlen(msg->StartLine.p2);
		strcpy(p, " ");
		p += 1;
		strcpy(p, msg->StartLine.p3);
		p += strlen(msg->StartLine.p3);
		strcpy(p, "\r\n");
		p += 2;

		for (i = 0; i < msg->header_cnt; i++) {
			strcpy(p, msg->headers[i].key);
			p += strlen(msg->headers[i].key);
			strcpy(p, ": ");
			p += 2;
			strcpy(p, msg->headers[i].value);
			p += strlen(msg->headers[i].value);
			strcpy(p, "\r\n");
			p += 2;
		}

		strcpy(p, "\r\n");
		p += 2;

		memcpy(p, msg->body, msg->bodylen);
	}
}
