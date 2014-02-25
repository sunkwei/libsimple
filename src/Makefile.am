include $(top_srcdir)/common.am

lib_LIBRARIES=libsimple.a

libsimple_a_SOURCES= circ_buf.c mutex.c semaphore.c \
		     thread.c threadpool.c tcpclient.c \
		     httpc.c httpparser.c fd.c	\
		     urlparser.c
libsimple_a_CFLAGS= -g -fPIC

if WIN32
libsimple_a_SOURCES+= regex.c
libsimple_a_CFLAGS+= -DSTDC_HEADERS
endif

if OSX
libsimple_a_SOURCES+= sem_timedwait.c
endif

