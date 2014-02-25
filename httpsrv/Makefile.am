include $(top_srcdir)/common.am

libsimple=$(top_builddir)/src/libsimple.a

bin_PROGRAMS=	httpsrv_bin

httpsrv_bin_SOURCES=	server.c client.c httprequest.c
httpsrv_bin_LDADD=	$(libsimple) 

