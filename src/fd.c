#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <fcntl.h>
#include <errno.h>
#ifdef WIN32
#  include <WinSock2.h>
#else
#  include <unistd.h>
#  include <sys/types.h>
#  include <sys/socket.h>
   typedef int SOCKET;
#  define closesocket close
#endif // os
#include "fd_impl.h"

static int _file_read(fd_t *fd, void *buf, int size)
{
	int rc = fread(buf, 1, size, fd->fd.fp);
	fd->err = errno;
	return rc;
}

static int _file_write(fd_t *fd, const void *data, int len)
{
	int rc = fwrite(data, 1, len, fd->fd.fp);
	fd->err = errno;
	return rc;
}

static int _file_eof(fd_t *fd)
{
	return feof(fd->fd.fp);
}

static int _lasterr(fd_t *fd)
{
	return fd->err;
}

static int _file_set_nonblock(fd_t *fd, int enable)
{
	return -1;
}

static int _file_set_read_buf(fd_t *fd, int len)
{
	return -1;
}

static int _file_set_write_buf(fd_t *fd, int len)
{
	return -1;
}

static int _sock_read(fd_t *fd, void *buf, int size)
{
	int rc = recv(fd->fd.sock, (char*)buf, size, 0);
	if (rc == -1) {
#ifdef WIN32
		fd->err = WSAGetLastError();
#else
		fd->err = errno;
#endif // 
	}

	return rc;
}

static int _sock_write(fd_t *fd, const void *data, int len)
{
	int rc = send(fd->fd.sock, (const char*)data, len, 0);
	if (rc == -1) {
#ifdef WIN32
		fd->err = WSAGetLastError();
#else
		fd->err = errno;
#endif // 
	}

	return rc;
}

static int _sock_eof(fd_t *fd)
{
	/** FIXME: 应该设置为非阻塞模式i */
	char c;
	int rc = recv(fd->fd.sock, &c, 1, MSG_PEEK);
	return rc == 0;
}

static int _sock_set_nonblock(fd_t *fd, int enable)
{
#ifdef WIN32
	unsigned long mode = enable ? 1 : 0;
	return ioctlsocket(fd->fd.sock, FIONBIO, &mode);
#else
	int flags = fcntl(fd->fd.sock, F_GETFL, 0);
	if (flags < 0) return -1;
	flags = enable ? (flags | O_NONBLOCK) : (flags &~ O_NONBLOCK);
	return fcntl(fd->fd.sock, F_SETFL, flags);
#endif // os
}

static int _sock_set_read_buf(fd_t *fd, int len)
{
	return setsockopt(fd->fd.sock, SOL_SOCKET, SO_RCVBUF, (const char*)&len, sizeof(len));
}

static int _sock_set_write_buf(fd_t *fd, int len)
{
	return setsockopt(fd->fd.sock, SOL_SOCKET, SO_SNDBUF, (const char*)&len, sizeof(len));
}

fd_t *simple_fd_open_from_file(FILE *fp)
{
	fd_t *fd = (fd_t*)malloc(sizeof(fd_t));
	fd->type = TYPE_FILE;
	fd->err = 0;
	fd->fd.fp = fp;
	fd->read = _file_read;
	fd->write = _file_write;
	fd->is_eof = _file_eof;
	fd->set_nonblock = _file_set_nonblock;
	fd->lasterr = _lasterr;

	return fd;
}

fd_t *simple_fd_open_from_socket(int sock)
{
	fd_t *fd = (fd_t*)malloc(sizeof(fd_t));
	fd->type = TYPE_SOCKET;
	fd->err = 0;
	fd->fd.sock = (SOCKET)sock;
	fd->read = _sock_read;
	fd->write = _sock_write;
	fd->is_eof = _sock_eof;
	fd->set_nonblock = _sock_set_nonblock;
	fd->lasterr = _lasterr;

	return fd;
}

void simple_fd_close(fd_t *fd)
{
	free(fd);
}
