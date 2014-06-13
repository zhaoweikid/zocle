#ifndef ZOCLE_HTTP_HTTPCLIEHT_H
#define ZOCLE_HTTP_HTTPCLIEHT_H

#include <stdio.h>
#include <zocle/internet/http/httpheader.h>
#include <zocle/internet/http/httpconn.h>
#include <zocle/internet/http/httpreq.h>
#include <zocle/internet/http/httpresp.h>

typedef struct _zc_httpclient
{
	zcHttpConn *conn;
	//zcHttpReq  *req;
	zcHttpResp *resp;
	uint16_t	retry;  // retry count
	uint16_t    client; // client user count
	uint8_t		threads; // default 1
	uint8_t		lost_continue; // default 1
}zcHttpClient;

zcHttpClient* zc_httpclient_new();
void		  zc_httpclient_delete(void*);

int			  zc_httpclient_open(zcHttpClient *, zcHttpReq *req);

#endif
