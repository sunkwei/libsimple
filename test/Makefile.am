include $(top_srcdir)/common.am

libsimple=$(top_builddir)/src/libsimple.a

bin_PROGRAMS=test_threadpool

test_threadpool_SOURCES= test_threadpool.c
test_threadpool_LDADD= $(libsimple) -lpthread
