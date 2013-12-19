#ifndef _simple_tcp_server__hh
#define _simple_tcp_server__hh

#ifdef WIN32
#  define FD_SETSIZE 1024
#  include <winsock2.h>
typedef int socklen_t;
#else
#  include <sys/socket.h>
#  include <sys/types.h>
#  include <netinet/ip.h>
#  include <netdb.h>
#endif // os

#include "fd.h"
#include "stream.h"

typedef struct tcpserver_t tcpserver_t;

/** tcp server 可能的回调，
 */
typedef struct tcpserver_callbacks
{
	/** 当有 accept 请求调用，如果返回 -1，则将关闭该链接.
			@param opaque: 此时调用者可以保存一个自己定义的指针，在 cb_read/cb_write 时，将交给调用者.
			@warning: 如果不设置该功能，链接将直接被关闭.
	 */
	int (*cb_open)(tcpserver_t *ts, void **opaque, const struct sockaddr *addrfrom, socklen_t addrlen);
	
	/** 当有数据需要读取时调用 
			一般通过：
				char buf[1024];
				int rc = simple_stream_read(s, buf, sizeof(buf));
				...
	 */
	void (*cb_read)(tcpserver_t *ts, void *opaque, stream_t *s);

	/** 当写缓冲可用时调用 
			一般通过：
				int rc = simple_stream_write(s, data, datalen);
				...
	 */
	void (*cb_write)(tcpserver_t *ts, void *opaque, stream_t *s);

	/** 当对方关闭连接时调用.
			只是作为通知.
	 */
	void (*cb_close)(tcpserver_t *ts, void *opaque);

} tcpserver_callbacks;

tcpserver_t *simple_tcpserver_open(int port, const tcpserver_callbacks *cbs, const char *bindip);
void simple_tcpserver_close(tcpserver_t *s);

/** 启用/禁用 cb_write() 回调
	一般在执行 simple_stream_write() 返回时 EAGAIN 时，启用，
	simple_stream_write() 成功后，禁用
 */
int simple_tcpserver_enable_write_callback(tcpserver_t *ts, stream_t *s, int enable);

/** 调用者负责驱动 select
    一般启动一个工作线程
	  while (!quit) {
	      simple_tcp_server_runonce(ts);
	  }
 */
void simple_tcpserver_runonce(tcpserver_t *s);

void simple_tcpserver_run(tcpserver_t *s, int *quit);

#endif // tcpserver.h
