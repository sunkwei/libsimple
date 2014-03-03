include $(top_srcdir)/common.am

libsimple=$(top_builddir)/src/libsimple.la

bin_PROGRAMS=test_threadpool_bin test_tcpclient_bin \
	     test_httpparser_bin

test_threadpool_bin_SOURCES= test_threadpool.c
test_threadpool_bin_LDADD= $(libsimple)

test_tcpclient_bin_SOURCES= test_tcpclient.c
test_tcpclient_bin_LDADD= $(libsimple) 

test_httpparser_bin_SOURCES= test_httpparser.c
test_httpparser_bin_LDADD= $(libsimple)
