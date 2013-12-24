#pragma once

#include "../include/simple/url.h"
#include "../include/simple/httpparser.h"
#include "client.h"

/** ���� http req, ����������浽 res ��.

	��һ��������ǣ�ʵ��һ�� url->path ƥ������� path ƥ�䣬���д���.
 */
int http_request(client *c, url_t *url, const HttpMessage *req, HttpMessage *res);
