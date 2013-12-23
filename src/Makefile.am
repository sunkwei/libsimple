include $(top_srcdir)/common.am

lib_LIBRARIES=libsimple.a

libsimple_a_SOURCES= circ_buf.c mutex.c semaphore.c \
		     thread.c threadpool.c tcpserver.c tcpclient.c \
		     httpc.c httpparser.c stream.c fd.c \
		     httpserver.c url.c
libsimple_a_CFLAGS= -g -fPIC

