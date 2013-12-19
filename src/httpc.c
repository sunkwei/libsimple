#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "httpc_impl.h"

HttpMessage *httpc_Message_create()
{
	HttpMessage *msg = (HttpMessage *)malloc(sizeof(HttpMessage));

	msg->StartLine.p1 = 0;
	msg->StartLine.p2 = 0;
	msg->StartLine.p3 = 0;

	msg->pri_data = 0;

	list_init(&msg->headers);
	msg->header_cnt = 0;

	msg->body = 0;
	msg->bodylen = 0;

	return msg;
}

extern void httpc_internal_releaseParserData(void *pridata);

void httpc_Message_release (HttpMessage *msg)
{
	list_head *pos, *n;

	safefree(msg->StartLine.p1);
	safefree(msg->StartLine.p2);
	safefree(msg->StartLine.p3);

	list_for_each_safe(pos, n, &msg->headers) {
		HttpHeader *hh = (HttpHeader*)pos;
		safefree(hh->key);
		safefree(hh->value);
		list_del(pos);
		free(hh);
	}
	msg->header_cnt = 0;

	safefree(msg->body);
	msg->bodylen = 0;

	httpc_internal_releaseParserData(msg->pri_data);
	msg->pri_data = 0;
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

static list_head *_make_kvpair(const char *key, const char *value)
{
	HttpHeader *h = (HttpHeader*)malloc(sizeof(HttpHeader));
	h->key = strdup(key);
	h->value = strdup(value);

	return (list_head*)h;
}

int httpc_Message_setValue (HttpMessage *msg,
		const char *key, const char *value)
{
	list_head *newkv = 0;

	/** FIXME: 这里直接新建一个 kv 对，但有可能不允许多值？？ */
	newkv = _make_kvpair(key, value);
	list_add(newkv, &msg->headers);
	msg->header_cnt++;

	return 0;
}

int httpc_Message_getValue (HttpMessage *msg,
		const char *key, const char **value)
{
	list_head *pos;

	*value = 0;

	list_for_each(pos, &msg->headers) {
		HttpHeader *h = (HttpHeader*)pos;
		if (!strcasecmp(h->key, key)) {
			*value = h->value;
			return 1;
		}
	}

	return 0;
}

int httpc_Message_delValue (HttpMessage *msg, const char *key)
{
	/** FIXME: 删除所有匹配的 !!! */
	list_head *pos, *n;

	list_for_each_safe(pos, n, &msg->headers) {
		HttpHeader *h = (HttpHeader*)pos;
		if (!strcasecmp(h->key, key)) {
			safefree(h->key);
			safefree(h->value);

			list_del(pos);

			free(h);
		}
	}

	return 0;
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
	int len = 0;
	list_head *pos;

	if (!msg || !msg->StartLine.p1 || !msg->StartLine.p2 || !msg->StartLine.p3) return -1;

	/** start line */
	len += strlen(msg->StartLine.p1) + 1; // ' '
	len += strlen(msg->StartLine.p2) + 1; // ' '
	len += strlen(msg->StartLine.p3);
	len += 2;	// \r\n

	/** 所有 headers 占用空间 */
	list_for_each(pos, &msg->headers) {
		HttpHeader *h = (HttpHeader*)pos;
		len += strlen(h->key) + 2;	// ': '
		len += strlen(h->value);
		len += 2;	// \r\n
	}

	/** 空行 */
	len += 2;	// \r\n

	/** body len */
	len += msg->bodylen;

	return len + 1;	// 呵呵，如果没有 body，可能需要 0 结束.
}

void httpc_Message_encode(HttpMessage *msg, char *buf)
{
	char *p = buf;
	list_head *pos;

	/** start line */
	p += sprintf(p, "%s %s %s\r\n", msg->StartLine.p1, msg->StartLine.p2, msg->StartLine.p3);

	/** all headers */
	list_for_each(pos, &msg->headers) {
		HttpHeader *h = (HttpHeader*)pos;
		p += sprintf(p, "%s: %s\r\n", h->key, h->value);
	}

	/** 空行 */
	strcpy(p, "\r\n");
	p += 2;

	memcpy(p, msg->body, msg->bodylen);
}
