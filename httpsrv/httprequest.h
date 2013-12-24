#pragma once

#include "../include/simple/url.h"
#include "../include/simple/httpparser.h"
#include "client.h"

/** 处理 http req, 并将结果保存到 res 中.

	更一般的做法是：实现一个 url->path 匹配表，根据 path 匹配，进行处理.
 */
int http_request(client *c, url_t *url, const HttpMessage *req, HttpMessage *res);
