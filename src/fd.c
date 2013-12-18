#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#ifdef WIN32
#  include <WinSock2.h>
#else
#  include <unistd.h>
   typedef int SOCKET;
#  define closesocket close
#endif // os
#include "fd_impl.h"

static int _file_read(fd_t *fd, void *buf, int size)
{
	return fread(buf, 1, size, fd->fd.fp);
}

static int _sock_read(fd_t *fd, void *buf, int size)
{
	return recv(fd->fd.sock, (char*)buf, size, 0);
}

static int _file_write(fd_t *fd, const void *data, int len)
{
	return fwrite(data, 1, len, fd->fd.fp);
}

static int _sock_write(fd_t *fd, const void *data, int len)
{
	return send(fd->fd.sock, (const char*)data, len, 0);
}

fd_t *simple_fd_open_from_file(FILE *fp)
{
	fd_t *fd = (fd_t*)malloc(sizeof(fd_t));
	fd->type = TYPE_FILE;
	fd->fd.fp = fp;
	fd->read = _file_read;
	fd->write = _file_write;

	return fd;
}

fd_t *simple_fd_open_from_socket(int sock)
{
	fd_t *fd = (fd_t*)malloc(sizeof(fd_t));
	fd->type = TYPE_SOCKET;
	fd->fd.sock = (SOCKET)sock;
	fd->read = _sock_read;
	fd->write = _sock_write;

	return fd;
}

void simple_fd_close(fd_t *fd)
{
	free(fd);
}
