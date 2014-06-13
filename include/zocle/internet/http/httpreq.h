#ifndef ZOCLE_HTTP_HTTPREQ_H
#define ZOCLE_HTTP_HTTPREQ_H

#include <zocle/ds/dict.h>
#include <zocle/str/string.h>
#include <zocle/str/cstrlist.h>
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
int    zc_url_domain2ip(zcURL *, zcCStrList *);
void   zc_url_print(zcURL*);
bool   zc_url_domain_isip(zcURL*);
bool   zc_host_isip(const char *);

typedef struct _zc_httpreq
{
	zcDict      *header;
	zcDict      *cookie;
	zcString	body;
	zcURL		url;
	uint8_t		method;
	uint8_t		protocol;
	uint8_t		websocket; // is websocket?
}zcHttpReq;

zcHttpReq* zc_httpreq_new(const char *urlstr);
void	   zc_httpreq_delete(void *);
int		   zc_httpreq_header_tostr(zcHttpReq*, zcString *s);
int		   zc_httpreq_form(zcHttpReq*, zcDict *form);
int		   zc_httpreq_header_websocket(zcHttpReq *);

#endif
