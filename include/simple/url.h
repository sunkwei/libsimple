#ifndef _simple_url__hh
#define _simple_url__hh

#include "list.h"

typedef struct key_value_t
{
	list_head head;	// 链表.

	char *key;
	char *value;

} key_value_t;

/** schema://authority/path?query#fragment */
typedef struct url_t
{
	char *schema;		// http, tcp, ...
	char *username;
	char *passwd;
	char *host;
	char *port;		// default "80"
	char *path;
	char *query;
	char *fragment;

	list_head query_pairs;	// 链表指向 key value 
} url_t;

/** 根据 src 解析得到 url_t 结构，必须使用 simple_url_release() 释放 */
url_t *simple_url_parse(const char *str);
void simple_url_release(url_t *u);

#endif // url.h
