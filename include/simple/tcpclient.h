#ifndef simple_tcp_client__hh
#define simple_tcp_client__hh

typedef struct tcpclient_t tcpclient_t;

/** 打开到 server:port 的 tcp 链接 
	@param timeout: 超时，3000 对应 3秒
 */
tcpclient_t *simple_tcpclient_open(const char *server, int port, int timeout);
void simple_tcpclient_close(tcpclient_t *tc);

/** 返回 -1: api 错误，-2 超时 */
int simple_tcpclient_send(tcpclient_t *tc, const void *data, int len, int *sent);
int simple_tcpclient_sendt(tcpclient_t *tc, const void *data, int len, int *sent, int timeout);
int simple_tcpclient_sendn(tcpclient_t *tc, const void *data, int len, int *sent);
int simple_tcpclient_sendnt(tcpclient_t *tc, const void *data, int len, int *sent, int timeout);

/** 返回 -1: api 错误，-2 超时 */
int simple_tcpclient_recv(tcpclient_t *tc, void *buf, int len, int *recved);
int simple_tcpclient_recvt(tcpclient_t *tc, void *buf, int len, int *recved, int timeout);
int simple_tcpclient_recvn(tcpclient_t *tc, void *buf, int len, int *recved);
int simple_tcpclient_recvnt(tcpclient_t *tc, void *buf, int len, int *recved, int timeout);

#endif // tcpclient.h
