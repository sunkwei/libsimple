#ifndef _simple_fd__hh
#define _simple_fd__hh

/** 对应描述文件，socket，...
 */
typedef struct fd_t fd_t;

fd_t *simple_fd_open_from_socket(int sock);
fd_t *simple_fd_open_from_file(FILE *fp);

/** 注意，不会关闭原始 sock, fp */
void simple_fd_close(fd_t *fd);

#endif // fd.h
