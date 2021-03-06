#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([FULL-PACKAGE-NAME], [VERSION], [BUG-REPORT-ADDRESS])
AC_CONFIG_SRCDIR([src/threadpool.c])
AC_CONFIG_HEADERS([config.h])
AC_CANONICAL_TARGET
AC_CANONICAL_HOST
AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE

AC_CONFIG_FILES([Makefile])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_LIBTOOL
AC_LIBTOOL_WIN32_DLL

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([malloc.h memory.h stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC

## 
WIN32="no"
OSX="no"

case "${host}" in
	i[[35]]86-*-mingw32*) 
		WIN32="yes" 
		CFLAGS="$CFLAGS -D_WIN32_WINNT=0x501"
		LIBS="$LIBS -lws2_32"
		;;

	*-apple-darwin*)
		OSX="yes"
		LIBS="$LIBS -pthread "
		;;

	?*)
		LIBS="$LIBS -lpthread -lrt"
		;;
esac

AM_CONDITIONAL([WIN32], test "${WIN32}" = "yes")
AM_CONDITIONAL([OSX], test "${OSX}" = "yes")

AC_OUTPUT( [src/Makefile
	   test/Makefile
	   httpsrv/Makefile
	   ]
	 )
