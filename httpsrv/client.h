#pragma once

#include "../include/simple/threadpool.h"

/** 一个 client 对应着一次 http connection
 */
typedef struct client
{
	int sock;
	void (*handle)(task_t *task, void *opaque);	// 用于处理一次http链接.
} client;

client *build_client(int sock);
