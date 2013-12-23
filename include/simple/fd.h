#ifndef _simple_fd__hh
#define _simple_fd__hh

#include <stdio.h>

/** ��Ӧ�����ļ���socket��...
 */
typedef struct fd_t fd_t;


fd_t *simple_fd_open_from_socket(int sock);
fd_t *simple_fd_open_from_file(FILE *fp);

/** ע�⣬���ر�ԭʼ sock, fp */
void simple_fd_close(fd_t *fd);
void simple_fd_detach(fd_t *fd);


/** �������������� */
int simple_fd_lasterr(fd_t *fd);
const char *simple_fd_lasterr_string(fd_t *fd, int err);



#endif // fd.h
