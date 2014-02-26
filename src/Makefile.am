include $(top_srcdir)/common.am

lib_LTLIBRARIES=libsimple.la

libsimple_la_SOURCES= circ_buf.c mutex.c semaphore.c \
		     thread.c threadpool.c tcpclient.c \
		     httpc.c httpparser.c fd.c	\
		     urlparser.c
libsimple_la_CFLAGS= -g -fPIC

if WIN32
libsimple_la_SOURCES+= regex.c
libsimple_la_CFLAGS+= -DSTDC_HEADERS
endif

if OSX
libsimple_la_SOURCES+= sem_timedwait.c
endif

