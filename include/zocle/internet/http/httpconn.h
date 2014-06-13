#ifndef ZOCLE_HTTP_HTTPCONN_H
#define ZOCLE_HTTP_HTTPCONN_H

#include <stdint.h>
#include <zocle/net/sockets.h>
#include <zocle/internet/http/httpreq.h>
#include <zocle/internet/http/httpresp.h>
#include <zocle/str/buffer.h>
#include <zocle/enc/compress.h>

typedef struct 
{
	int64_t start;
	int64_t dns_time;
	int64_t conn_time;
	int64_t ssl_time;
	int64_t send_time;
	int64_t recv_time;
	int64_t recv_first_time;
	int64_t all_time;
}zcHttpConnStat;

void zc_httpconnstat_print(zcHttpConnStat *stat);

typedef struct _zc_httpconn
{
	zcSocket *sock;
	char	 host[16];
	int32_t  port;
	int32_t	 conn_timeout;
	int32_t	 read_timeout;
	int32_t	 write_timeout;
	int		 (*readfunc)(const char *buf, int len);  // for recv body in response
	int		 (*writefunc)(const char *buf, int len); // for send body in request
	zcHttpConnStat *stat;
}zcHttpConn;

zcHttpConn* zc_httpconn_new();
void		zc_httpconn_delete(void *);

int			zc_httpconn_open(zcHttpConn*, const char *host, int port, bool isssl);
int			zc_httpconn_send(zcHttpConn*, zcHttpReq*);
zcHttpResp*	zc_httpconn_recv(zcHttpConn*);

int			zc_httpconn_gzip_check_header(const char *s, int slen);
int			zc_httpconn_gzip_uncompress(zcHttpResp *resp, zcBuffer *buf, zcCompress *cm, bool *first);
int			zc_httpconn_gzip_uncompress_str(zcHttpResp *resp, char *buf, int blen, zcCompress *cm, bool *first);

#endif
