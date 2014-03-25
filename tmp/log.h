#pragma  once

#include <stdio.h>
#include <stdlib.h>

inline void mylog_init()
{
	FILE *fp = fopen("tdt.log", "w");
	if (fp) {
		fclose(fp);
	}
}

inline int mylog(const char *fmt, ...)
{
	int rc = 0;
	FILE *fp = fopen("tdt.log", "at");
	if (fp) {
		va_list mark;
		char buf[4096];
		va_start(mark, fmt);
		vsnprintf(buf, sizeof(buf), fmt, mark);
		va_end(mark);

		rc = fprintf(fp, buf);
		fclose(fp);

		fprintf(stdout, buf);
	}

	return rc;
}
