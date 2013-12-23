#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "regex.h"
#include "../include/simple/url.h"
#include "httpc_impl.h"

static char *strndup (const char *s, int n)
{
	if (n > 0) {
		char *rs = (char*)malloc(n+4);
		memset(rs, 0, n+4);
		strncpy(rs, s, n);
		rs[n] = 0;
		return rs;
	}
	else
		return strdup("");
}

//       ^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?
//        12            3  4          5       6  7        8 9
//
//  The numbers in the second line above are only to assist readability;
//  they indicate the reference points for each subexpression (i.e., each
//  paired parenthesis).  We refer to the value matched for subexpression
//  <n> as $<n>.  For example, matching the above expression to
//
//   http://www.ics.uci.edu/pub/ietf/uri/#Related
//
//  results in the following subexpression matches:
//
//  $1 = http:
//  $2 = http
//  $3 = //www.ics.uci.edu
//  $4 = www.ics.uci.edu
//  $5 = /pub/ietf/uri/
//  $6 = <undefined>
//  $7 = <undefined>
//  $8 = #Related
//  $9 = Related
//
//  where <undefined> indicates that the component is not present, as is
//  the case for the query component in the above example.  Therefore, we
//  can determine the value of the four components and fragment as
//
//  scheme    = $2
//  authority = $4
//  path      = $5
//  query     = $7
//  fragment  = $9

/** ½âÎö authority: user@passwd:host:port
	^(([^@]+)@([^:]+):)?([^:]+)(:([0..9]+))?
	 12       3         4      5 6 
	
	$1 = user@passwd
	$2 = user
	$3 = passwd
	$4 = host
	$5 = :port
	$6 = port
 */

static key_value_t *_parse_kv(char *p)
{
	/** key=value */
	const char *pos = strchr(p, '=');
	if (!pos) {
		return 0;
	}
	else {
		key_value_t *kv = (key_value_t*)malloc(sizeof(key_value_t));
		kv->key = strndup(p, (pos-p));
		kv->value = strdup(pos+1);

		return kv;
	}
}

static void _parse_query(const char *query, list_head *head)
{
	/**  key1=value1&key2=value2&key3= ... 
	 */
	char *str = strdup(query);
	char *p = strtok(str, "&");
	while (p) {
		key_value_t *kv = _parse_kv(p);
		if (kv) {
			list_add((list_head*)kv, head);
		}

		p = strtok(0, "&");
	}

	free(str);
}

url_t *simple_url_parse(const char *str)
{
	static regex_t _reg_url, _reg_auth;
	static int _inited = 0;
	static const char *exp_url = "^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)?(\\?([^#]*))?(#(.*))?";
	static const char *exp_authority = "^(([^@]+)@([^:]+):)?([^:]+)(:([0-9]+))?";
	regmatch_t subs[10];
	int rc;

	if (!_inited) {
		rc = regcomp(&_reg_url, exp_url, REG_ICASE | REG_EXTENDED);
		rc = regcomp(&_reg_auth, exp_authority, REG_ICASE | REG_EXTENDED);
		_inited = 1;
	}

	rc = regexec(&_reg_url, str, 10, subs, 0);
	if (rc == 0) {
		url_t *url = (url_t*)malloc(sizeof(url_t));
		char *authority = strndup(str + subs[4].rm_so, subs[4].rm_eo - subs[4].rm_so);

		url->schema = strndup(str + subs[2].rm_so, subs[2].rm_eo - subs[2].rm_so);
		url->path = strndup(str + subs[5].rm_so, subs[5].rm_eo-subs[5].rm_so);
		url->query = strndup(str + subs[7].rm_so, subs[7].rm_eo-subs[7].rm_so);
		url->fragment = strndup(str + subs[9].rm_so, subs[9].rm_eo-subs[9].rm_so);

		list_init(&url->query_pairs);

		_parse_query(url->query, &url->query_pairs);

		rc = regexec(&_reg_auth, authority, 10, subs, 0);
		if (rc == 0) {
			url->username = strndup(authority + subs[2].rm_so, subs[2].rm_eo - subs[2].rm_so);
			url->passwd = strndup(authority + subs[3].rm_so, subs[3].rm_eo - subs[3].rm_so);
			url->host = strndup(authority + subs[4].rm_so, subs[4].rm_eo - subs[4].rm_so);
			if (subs[6].rm_so == -1)
				url->port = strdup("80");
			else
				url->port = strndup(authority + subs[6].rm_so, subs[6].rm_eo - subs[6].rm_so);
		}
		else {
			url->username = strdup("");
			url->passwd = strdup("");
			url->host = strdup("");
			url->port = strdup("80");
		}

		free(authority);

		return url;
	}

	return 0;
}

void simple_url_release(url_t *u)
{
	list_head *pos, *n;

	list_for_each_safe(pos, n, &u->query_pairs) {
		key_value_t *kv = (key_value_t*)pos;
		free(kv->key);
		free(kv->value);
		list_del(pos);
		free(kv);
	}

	free(u->username);
	free(u->passwd);
	free(u->host);
	free(u->port);
	free(u->fragment);
	free(u->path);
	free(u->query);
	free(u->schema);

	free(u);
}
