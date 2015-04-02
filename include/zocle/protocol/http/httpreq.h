#ifndef ZOCLE_HTTP_HTTPREQ_H
#define ZOCLE_HTTP_HTTPREQ_H

#include <zocle/ds/dict.h>
#include <zocle/str/string.h>
#include <zocle/protocol/http/url.h>
#include <stdint.h>

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
