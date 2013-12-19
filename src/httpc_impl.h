#ifndef httpc_impl__hh
#define httpc_impl__hh

#ifdef WIN32
#  define strcasecmp _stricmp
#  define snprintf _snprintf
#  define strdup _strdup
#endif // win32

#include "../include/simple/httpparser.h"

#define safefree(x) {	\
	if (x) free(x);		\
	x = 0;				\
}

// private data for parser
typedef struct ParserData
{
	char *linedata;
	int line_bufsize;
	int line_datasize;

	HttpParserState state, last_state;
} ParserData;

#endif // httpc_impl.h
