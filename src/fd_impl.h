#ifndef __fd_impl__hh
#define __fd_impl__hh

#include "../include/simple/fd.h"

typedef enum FDTYPE
{
	TYPE_SOCKET,
	TYPE_FILE,
} FDTYPE;

struct fd_t
{
	union {
		SOCKET sock;
		FILE *fp;
	} fd;
	FDTYPE type;

	int (*read)(fd_t *fd, void *buf, int bufsize);
	int (*write)(fd_t *fd, const void *data, int datalen);
	int (*is_eof)(fd_t *fd);	// ����Ƿ���� .
	int (*set_nonblock)(fd_t *fd, int enable);	// ������������������Ŀǰ������ socket ��Ч ..
	int (*set_read_buf)(fd_t *fd, int size);	// ���û����С��Ŀǰ���� socket ʵ�� ...
	int (*set_write_buf)(fd_t *fd, int size);
};

#endif // fd_impl.h
