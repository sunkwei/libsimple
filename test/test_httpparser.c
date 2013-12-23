#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/simple/httpparser.h"
#include "../include/simple/url.h"

int main()
{
	const char *str1 = 
		"GET /index.html HTTP/1.0\r\n"
		"Connection: close\r\n"
		"agent: test\r\n"
		"\r\n"
		;

	const char *str2 = 
		"POST http://172.16.1.10:8000/action.html?act=doit&param=123&param2= HTTP/1.0\r\n"
		"test : abcd\r\n"	// test lsw
		" efg \r\n"
		"Connection: close \r\n"
		"\r\n";

	HttpMessage *msg = 0;
	int used = 0;
	char *encode_buf = 0;
	url_t *url = 0;

	msg = httpc_parser_parse(msg, str1, strlen(str1), &used);
	assert(httpc_Message_state(msg) == HTTP_COMPLETE);
	httpc_Message_release(msg);

	msg = httpc_parser_parse(0, str2, strlen(str2), &used);
	assert(httpc_Message_state(msg) == HTTP_COMPLETE);

	used = httpc_Message_get_encode_length(msg);
	encode_buf = (char*)malloc(used);
	httpc_Message_encode(msg, encode_buf);

	url = simple_url_parse(msg->StartLine.p2);
	if (url) {
		simple_url_release(url);
	}

	httpc_Message_release(msg);

	fprintf(stdout, encode_buf);
	free(encode_buf);

	return 0;
}
