#ifndef __circ_buf__hh
#define __circ_buf__hh

typedef struct circ_buf_t circ_buf_t;

////////////////////////////////////////////////////
circ_buf_t *simple_circ_buf_create(int size);
void simple_circ_buf_destroy(circ_buf_t *cb);

int simple_circ_buf_get_space_cnt(circ_buf_t *cb);
int simple_circ_buf_get_data_cnt(circ_buf_t *cb);

/** 保存，返回实际存储的字节数 */
int simple_circ_buf_write(circ_buf_t *cb, const void *data, int len);

/** 取出，返回实际得到的字节数 */
int simple_circ_buf_read(circ_buf_t *cb, void *buf, int buflen);

#endif // circ_buf.h
