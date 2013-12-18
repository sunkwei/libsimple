#ifndef __simple_stream__hh
#define __simple_stream__hh

#include "fd.h"

typedef struct stream_t stream_t;

/** 创建基于句柄的流对象 */
stream_t *simple_stream_open(fd_t *fd);
void simple_stream_close(stream_t *s);

/** 读 */
int simple_stream_read(stream_t *s, void *buf, int bufsize);

/** 写 */
int simple_stream_write(stream_t *s, const void *data, int size);

#endif // stream.h
