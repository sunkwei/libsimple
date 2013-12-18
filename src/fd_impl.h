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
};

#endif // fd_impl.h
