#ifndef ZOCLE_HTTP_URL_H
#define ZOCLE_HTTP_URL_H

#include <zocle/ds/dict.h>
#include <zocle/str/string.h>
#include <stdint.h>

typedef struct _zc_url
{
	zcString url;
	zcString protocol;
	zcString username;
	zcString password;
	zcString domain;
	zcString path;
	zcString params;
	zcString query;
	int		 port;
}zcURL;

zcURL* zc_url_new(const char *urlstr);
void   zc_url_delete(void *x);
void   zc_url_clear(zcURL *);
int    zc_url_init(zcURL *, const char *urlstr);
void   zc_url_destroy(void *x);
int    zc_url_parse(zcURL *, const char *urlstr);
int    zc_url_domain2ip(zcURL *, zcList *);
void   zc_url_print(zcURL*);
bool   zc_url_domain_isip(zcURL*);
bool   zc_host_isip(const char *);

#endif
