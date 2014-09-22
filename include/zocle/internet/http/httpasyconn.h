#ifdef ZOCLE_WITH_LIBEV
#ifndef ZOCLE_HTTP_HTTPASYCONN_H
#define ZOCLE_HTTP_HTTPASYCONN_H

#include <stdio.h>
#include <zocle/internet/http/httpreq.h>
#include <zocle/internet/http/httpresp.h>
#include <zocle/internet/http/httpconn.h>
#include <zocle/server/asynconn.h>
#include <zocle/enc/compress.h>

#define ZC_WS_AGAIN		0
#define ZC_WS_TEXT		1
#define ZC_WS_BIN		2
#define ZC_WS_CLOSE		8
#define ZC_WS_PING		9
#define ZC_WS_PONG		0x0a

#define ZC_WS_CLOSE_NO		0
#define ZC_WS_CLOSE_RECV	1
#define ZC_WS_CLOSE_SEND	2

typedef struct zc_httpinfo_t
{
	/*char	 host[16];
	int32_t  port;
	bool	 ssl:1;*/
	bool     header:1;
	bool	 longconn:1; 
	bool	 websocket:1;
	char	 websocket_close:3; // 0 not close, 1 recv close, 2 send close
	bool	 bodyfirst;
	int		 chunklen;
	int64_t  readlen;
	int32_t	 conn_timeout;
	int32_t	 read_timeout;
	int32_t	 write_timeout;
	int		 (*readfunc)(const char *buf, int len, bool finish, void *);  // for recv body in response
	int		 (*writefunc)(const char *buf, int len, void *); // for send body in request
	int		 (*readyfunc)(zcAsynConn*); // for whole package received
	zcHttpReq      *req;
	zcHttpResp     *resp;
	zcHttpConnStat *stat;
	zcCompress	   *compress;
	void		   *data;
	//uint64_t time;
}zcHttpInfo;

typedef int (*zcFuncHttpReady)(zcAsynConn*);

zcHttpInfo*	zc_httpinfo_new();
void		zc_httpinfo_delete(void *);

zcAsynConn* zc_asynconn_new_http(zcHttpInfo *info, int timeout, struct ev_loop *loop, const char *dns, int bufsize);
zcAsynConn* zc_asynconn_new_http_url(const char *url, int timeout, struct ev_loop *loop, 
				const char *dns, int bufsize, zcHttpInfo *info);

int zc_asynhttp_assign_conn(zcAsynConn *);
int zc_asynhttp_send(zcAsynConn *conn, zcHttpReq *req);
int zc_asynhttp_websocket_send(zcAsynConn *conn, uint8_t fin, uint8_t opcode, const char *data, int datalen);
int zc_asynhttp_switch_websocket(zcAsynConn *conn);

int zc_asynhttp_handle_connected(zcAsynConn*);
int zc_asynhttp_handle_websocket_read(zcAsynConn*, zcBuffer *);
int zc_asynhttp_handle_read_check(zcAsynConn*, zcBuffer *);
int zc_asynhttp_handle_ready(zcAsynConn*, char *data, int datalen);
int zc_asynhttp_handle_ready_dns(zcAsynConn*, char *data, int datalen);
int zc_asynhttp_handle_wrote(zcAsynConn*);

/*int zc_asynhttp_handle_read_timeout(zcAsynConn*);
int zc_asynhttp_handle_write_timeout(zcAsynConn*);*/

#endif
#endif
