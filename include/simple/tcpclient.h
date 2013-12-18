#ifndef simple_tcp_client__hh
#define simple_tcp_client__hh

typedef struct tcpclient_t tcpclient_t;

/** �򿪵� server:port �� tcp ���� 
	@param timeout: ��ʱ��3000 ��Ӧ 3��
 */
tcpclient_t *simple_tcpclient_open(const char *server, int port, int timeout);
void simple_tcpclient_close(tcpclient_t *tc);

/** ���� -1: api ����-2 ��ʱ */
int simple_tcpclient_send(tcpclient_t *tc, const void *data, int len, int *sent);
int simple_tcpclient_sendt(tcpclient_t *tc, const void *data, int len, int *sent, int timeout);
int simple_tcpclient_sendn(tcpclient_t *tc, const void *data, int len, int *sent);
int simple_tcpclient_sendnt(tcpclient_t *tc, const void *data, int len, int *sent, int timeout);

/** ���� -1: api ����-2 ��ʱ */
int simple_tcpclient_recv(tcpclient_t *tc, void *buf, int len, int *recved);
int simple_tcpclient_recvt(tcpclient_t *tc, void *buf, int len, int *recved, int timeout);
int simple_tcpclient_recvn(tcpclient_t *tc, void *buf, int len, int *recved);
int simple_tcpclient_recvnt(tcpclient_t *tc, void *buf, int len, int *recved, int timeout);

#endif // tcpclient.h
