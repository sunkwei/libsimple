#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <direct.h>
#ifdef WIN32
#  include <WinSock2.h>
#else
#endif // os
#include <sys/types.h>
#include <sys/stat.h>
#include "httprequest.h"

#ifdef WIN32
#  define strcasecmp stricmp
#endif // os

/** ���� root path��һ��Ӧ��ͨ�����÷�ʽ. */
static const char *root_path()
{
	static char curr[1024];
	getcwd(curr, sizeof(curr));

	return curr;	// ʹ�õ�ǰĿ¼.
}

/** ʹ��root + path ��Ϊ�ļ���� */
static int is_local_file(const char *filename)
{
	struct stat buf;
	if (stat(filename, &buf) == 0) {
		if (buf.st_mode & S_IFDIR) {
			// Ŀ¼����������.
			return 0;
		}

		if (buf.st_mode & S_IFMT) {
			// �ļ�.
			return 1;
		}
	}
	return 0;
}

typedef struct ext_content_type
{
	const char *ext;
	const char *type;
} ext_content_type;

static ext_content_type _types[] = 
{
	{ ".txt", "text/plain" },
	{ ".html", "text/html" },
	{ ".htm", "text/html" },
	{ ".ico", "image/x-icon" },
	{ ".jpg", "image/jpeg" },
	{ ".jpeg", "image/jpeg" },
	{ ".gif", "image/gif" },
	{ ".mp3", "audio/mp3" },
	{ ".mpeg", "video/mpg" },
	{ ".mp4", "video/mpeg" },
	{ ".js", "application/x-javascript" },
	{ ".java", "java/*" },
	{ ".png", "image/png" },
	{ ".gz", "application/x-gzip" },
	{ ".ppt", "application/mspowerpoint" },
	// .... ������Ӻܶ�ܶ� ...
	{ 0, 0 },
};

static const char *get_context_type_from_ext(const char *ext)
{
	ext_content_type *et = &_types[0];
	while (et->ext) {
		if (!strcasecmp(ext, et->ext)) {
			return et->type;
		}

		et++;
	}

	return "application/octet-stream";
}

static int send_data(int sock, const char *data, int len)
{
	while (len > 0) {
		int rc = send(sock, data, len, 0);
		if (rc > 0) {
			len -= rc;
			data += rc;
		}
		else {
			return -1;
		}
	}

	return 0;
}

/** ���ͱ����ļ� */
static int send_local_file(client *c, HttpMessage *res, const char *filename, url_t *url)
{
	/** �����ļ���չ�������� Content-Type ����. */
	const char *ext = strrchr(filename, '.');
	struct stat st;
	char vbuf[64];
	char *abuf;
	int alen;
	FILE *fp;
	int err = 0;

	stat(filename, &st);
	_snprintf(vbuf, sizeof(vbuf), "%u", st.st_size);

	httpc_Message_setValue(res, "Content-Type", get_context_type_from_ext(ext));
	httpc_Message_setValue(res, "Content-Length", vbuf);

	alen = httpc_Message_get_encode_length(res, 0);	// ������body ����
	abuf = (char*)alloca(alen+1);	// ʹ��ջ���� ...
	httpc_Message_encode(res, abuf, 0);

	if (send_data(c->sock, abuf, alen) < 0) {
		fprintf(stderr, "ERR: send file err\n");
		return -1;
	}

	fp = fopen(filename, "rb");
	if (!fp) {
		fprintf(stderr, "ERR: open file %s err\n", filename);
		return -1;
	}

	err = 0;
	while (!feof(fp)) {
		char buf[4096];
		int rc = fread(buf, 1, sizeof(buf), fp);
		if (rc > 0) {
			if (send_data(c->sock, buf, rc) < 0) {
				fprintf(stderr, "ERR: send file err\n");
				err = -1;
				break;
			}
		}
	}

	fclose(fp);

	return err;
}

static int ret_404(client *c, HttpMessage *res, const char *path)
{
	char *body = (char*)alloca(1024);

	_snprintf(body, 1024, "NOT found %s", path);

	httpc_Message_setStartLine(res, "HTTP/1.0", "404", "File Not Found");
	httpc_Message_setValue(res, "Content-Type", "text/plain");
	httpc_Message_appendBody(res, body, strlen(body)+1);	// strlen + 1 ��Ŀ�ı�֤���� 0����.

	return 0;
}

static void build_filename(char *buf, int size, const char *root, const char *path)
{
	// FIXME: ����Ӧ�ý��� path �п��ܵ� %xx%xx ...

	strcat(buf, root);
#ifdef WIN32
	while (*path) {
		if (*path == '/')
			strncat(buf, "\\", 1);
		else
			strncat(buf, path, 1);

		path++;
	}
#else
	strcat(buf, path);
#endif // os
}

/** һ�����͵� http server����Ҫ֧�� http authentication��һ��ͨ����� req �е� WWW-Authorization �ֶ� ...
    ������Դ�����.
 */
int http_request(client *c, url_t *url, const HttpMessage *req, HttpMessage *res)
{
	char *filename = (char*)alloca(1024);
	filename[0] = 0;

	build_filename(filename, 1024, root_path(), url->path);

	/** step1 ��� path �Ƿ�ƥ�䱾���ļ�?
	 */
	if (is_local_file(filename)) {
		return send_local_file(c, res, filename, url);
	}

	/** step2 ��ʱ�Ҳ���ƥ����ļ�������ִ��һЩ�ض��Ĳ��� */
	
	/** step3 ���� 404 */
	return ret_404(c, res, filename);
}
