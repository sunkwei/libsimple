include $(top_srcdir)/common.am

libsimple=$(top_builddir)/src/libsimple.a

bin_PROGRAMS=test_threadpool_bin test_tcpserver_bin test_tcpclient_bin

test_threadpool_bin_SOURCES= test_threadpool.c
test_threadpool_bin_LDADD= $(libsimple) -lpthread

test_tcpserver_bin_SOURCES= test_tcpserver.c
test_tcpserver_bin_LDADD= $(libsimple) -lpthread

test_tcpclient_bin_SOURCES= test_tcpclient.c
test_tcpclient_bin_LDADD= $(libsimple) -lpthread

