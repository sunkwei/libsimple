#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "httpc_impl.h"

#define EXPSIZE 128


void httpc_internal_releaseParserData(void *p)
{
	ParserData *data = (ParserData *)p;
	if (!p) return;

	if (data->linedata) {
		free(data->linedata);
	}

	free(data);
}

static int isblank(int c)
{
	if (c == ' ' || c == '\t')
		return 1;
	else
		return 0;
}

static int isspace(int c)
{
	if (isblank(c) || c == '\r' || c == '\n')
		return 1;
	else
		return 0;
}

static void _append(char c, HttpMessage *msg)
{
	ParserData *pd = (ParserData *)msg->pri_data;
	if (pd->line_datasize + 1 > pd->line_bufsize) {
		pd->line_bufsize += EXPSIZE;
		pd->linedata = (char *)realloc(pd->linedata, pd->line_bufsize);
	}
	pd->linedata[pd->line_datasize++] = c;
}

static int _getmsglen(HttpMessage *msg)
{
	const char *v = 0;
	if (httpc_Message_getValue(msg, "Content-Length", &v)) {
		return atoi(v);
	}
	else {
		return 0;
	}
}

static int parseStartLine(HttpMessage *msg)
{
	ParserData *pd = (ParserData *)msg->pri_data;
	const char *p = pd->linedata;
	const char *tail = pd->linedata + pd->line_datasize;
	int i;

	// method/version + url/code + version/status
	char *tmp = (char *)malloc(pd->line_datasize+1);

	// p1
	memset(tmp, 0, pd->line_datasize);
	for (i = 0; p < tail && !isspace(*p); p++, i++) {
		tmp[i] = *p;
	}
	safefree(msg->StartLine.p1);
	msg->StartLine.p1 = strdup(tmp);

	while (p < tail && isspace(*p)) p++;

	// p2
	memset(tmp, 0, pd->line_datasize);
	for (i = 0; p < tail && !isspace(*p); p++, i++) {
		tmp[i] = *p;
	}
	safefree(msg->StartLine.p2);
	msg->StartLine.p2 = strdup(tmp);

	while (p < tail && isspace(*p)) p++;

	// p3
	memset(tmp, 0, pd->line_datasize);
	for (i = 0; p < tail && *p != '\r' && *p != '\n'; p++, i++) {
		tmp[i] = *p;
	}
	safefree(msg->StartLine.p3);
	msg->StartLine.p3 = strdup(tmp);

	free(tmp);
	return 1;
}

static int parseHeaderLine(HttpMessage *msg)
{
	// using ":" to split key and value£¬from msg->pri_data->linedata
	ParserData *pd = (ParserData *)msg->pri_data;
	char *key = (char *)malloc(pd->line_datasize+1);
	char *value = (char *)malloc(pd->line_datasize+1);
	const char *p = pd->linedata;
	const char *tail = p+pd->line_datasize;
	const char *q = p;
	const char *x;
	while (p < tail && isspace(*p))
		p++;

	q = p;
	while (p < tail && *p != ':') p++;
	x = p+1;	// *x = ':'

	p--;
	while (p > q && isspace(*p)) p--;
	if (q == p) {
	}
	else {
		memcpy(key, q, p-q+1);
		key[p-q+1] = 0;
	}

	q = x;

	p = tail - 1;
	while (p > q && isspace(*p)) p--;
	while (*q && isspace(*q)) q++;

	memcpy(value, q, p-q+1);
	value[p-q+1] = 0;

	httpc_Message_setValue(msg, key, value);

	free(key);
	free(value);
	return 1;
}

static int appendByte(char c, HttpMessage *msg)
{
	int ret = 1;
	ParserData *pd = (ParserData *)msg->pri_data;

	switch (pd->state) {
		case HTTP_STARTLINE:
			_append(c, msg);
			if (c == '\r') {
				pd->last_state = HTTP_STARTLINE;
				pd->state = HTTP_CR;
			}
			break;

		case HTTP_HEADERS:
			_append(c, msg);
			if (c == '\r') {
				pd->last_state = HTTP_HEADERS;
				pd->state = HTTP_CR;
			}
			break;

		case HTTP_CR:
			_append(c, msg);
			if (c == '\n') {
				pd->state = HTTP_CRLF;

				// blank line ?
				if (!strncmp(pd->linedata, "\r\n", 2)) {
					if (_getmsglen(msg) > 0) {
						pd->state = HTTP_BODY;
					}
					else {
						pd->state = HTTP_COMPLETE;
					}
				}
			}
			else {
				pd->state = pd->last_state;
			}
			break;

		case HTTP_CRLF:
			if (isblank(c)) {
				char *p;
				// LSW
				pd->state = HTTP_LSW;

				p = strstr(pd->linedata, "\r\n");
				if (p) {
					pd->line_datasize = p-pd->linedata;
				}
			}
			else {
				if (pd->last_state == HTTP_STARTLINE) {
					parseStartLine(msg);
				}
				else {
					parseHeaderLine(msg);
				}

				pd->line_datasize = 0;
				_append(c, msg);

				if (c == '\r')
					pd->state = HTTP_CR;
				else
					pd->state = HTTP_HEADERS;
			}
			break;

		case HTTP_LSW:
			if (!isblank(c)) {
				pd->state = pd->last_state;
				_append(c, msg);
			}
			break;

		case HTTP_BODY:
			msg->body = (char *)realloc(msg->body, msg->bodylen+1);
			msg->body[msg->bodylen] = c;
			msg->bodylen++;
			if (_getmsglen(msg) == msg->bodylen)
				pd->state = HTTP_COMPLETE;
			break;

		case HTTP_COMPLETE:
			break;
	}

	return ret;
}

static int appendBody(const char *p, int len, HttpMessage *msg)
{
	msg->body = (char *)realloc(msg->body, msg->bodylen+len);
	memcpy(&msg->body[msg->bodylen], p, len);
	msg->bodylen += len;

	if (_getmsglen(msg) == msg->bodylen) {
		ParserData *pd = (ParserData *)msg->pri_data;
		pd->state = HTTP_COMPLETE;
	}

	return len;
}

HttpMessage *httpc_parser_parse(HttpMessage *saved, 
		const char *data, int len, int *used)
{
	ParserData *pd;
	HttpMessage *msg = saved;
	const char *p = data;

	*used = 0;

	if (!msg) {
		msg = httpc_Message_create();
		msg->pri_data = malloc(sizeof(ParserData));
		pd = (ParserData *)msg->pri_data;
		pd->linedata = 0;
		pd->line_bufsize = 0;
		pd->line_datasize = 0;
		pd->state = HTTP_STARTLINE;
	}
	else {
		pd = (ParserData *)msg->pri_data;
	}

	while (pd->state != HTTP_COMPLETE && len > 0) {
		if (pd->state == HTTP_BODY) {
			const char *v = 0;
			int rc;
			int bodylen, expect;

			httpc_Message_getValue(msg, "Content-Length", &v);
			bodylen = atoi(v);
			expect = bodylen - msg->bodylen;
			rc = appendBody(p, len > expect ? expect : len, msg);
			p += rc;
			len -= rc;
			*used += rc;
		}
		else {
			int rc = appendByte(*p, msg);
			p += rc;
			len -= rc;
			*used += rc;
		}
	}

	return msg;
}

HttpParserState httpc_Message_state(HttpMessage *msg)
{
	if (msg && msg->pri_data) {
		ParserData *pd = (ParserData *)msg->pri_data;
		return pd->state;
	}
	else
		return HTTP_UNKNOWN;
}
