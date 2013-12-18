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
	int (*is_eof)(fd_t *fd);	// 检查是否结束 .
	int (*set_nonblock)(fd_t *fd, int enable);	// 设置阻塞，非阻塞，目前仅仅对 socket 有效 ..
	int (*set_read_buf)(fd_t *fd, int size);	// 设置缓冲大小，目前仅仅 socket 实现 ...
	int (*set_write_buf)(fd_t *fd, int size);
};

#endif // fd_impl.h
