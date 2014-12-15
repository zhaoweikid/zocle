#ifndef ZOCLE_HTTP_HTTPRESP_H
#define ZOCLE_HTTP_HTTPRESP_H

#include <stdint.h>
#include <zocle/str/string.h>
#include <zocle/ds/dict.h>
#include <zocle/protocol/http/cookie.h>
#include <zocle/protocol/http/httpreq.h>

typedef struct _zc_httpresp
{
	zcString headdata;
	zcString bodydata;
	int64_t  bodylen;
	short    code;
	zcString msg;
	zcString protocol;
	zcDict  *header;
	zcDict	*cookie;
	uint8_t	 compress:3;
	uint8_t	 keepalive:1;
	uint8_t  chunked:1;
	uint8_t  isfinish:1;
}zcHttpResp;

zcHttpResp* zc_httpresp_new();
void		zc_httpresp_delete(void *);
int			zc_httpresp_parse(zcHttpResp *);
int			zc_httpresp_parse_header(zcHttpResp *);
int			zc_httpresp_cookie_toreq(zcHttpResp *resp, zcHttpReq *req);
int			zc_httpresp_check_websocket(zcHttpResp*, zcHttpReq *);

#endif
