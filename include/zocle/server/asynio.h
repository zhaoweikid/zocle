#ifdef ZOCLE_WITH_LIBEV

#ifndef ZOCLE_SERVER_ASYNIO_H
#define ZOCLE_SERVER_ASYNIO_H

#include <zocle/net/sockets.h>
#include <zocle/str/buffer.h>
#include <ev.h>

struct zc_asynio_t;
typedef struct zc_asynio_t zcAsynIO;

struct zc_callchain_t;
typedef struct zc_callchain_t zcCallChain;
typedef int (*zcFuncCallBack)(zcAsynIO*, zcCallChain *chain, int ret);
typedef int (*zcFuncReadx)(zcAsynIO*, const char*, int ret);

#define ZC_CALL_ONCE	1
#define ZC_CALL_ALWAYS	2

typedef struct zc_callitem_t
{
    struct zc_callitem_t *next;
	zcFuncCallBack func;
	zcFuncCallBack errfunc;
	void  *data;
}zcCallItem;

struct zc_callchain_t
{
	//char type;  // 1 once, 2 always
	zcCallItem *items;
	zcCallItem *cur;
};

zcCallChain* zc_callchain_new();
void		 zc_callchain_delete(void*);
int			 zc_callchain_append(zcCallChain*, zcFuncCallBack f, zcFuncCallBack errf, void *data);
int			 zc_callchain_prepend(zcCallChain*, zcFuncCallBack f, zcFuncCallBack errf, void *data);
int			 zc_callchain_call(zcCallChain*, zcAsynIO*, int);
#define		 zc_callchain_reset(x) x->cur=x->items
#define      zc_callchain_safedel(x)    do{zc_callchain_delete(x);x=NULL;}while(0)

typedef struct zc_protocol_t
{
    int (*handle_accept)(zcAsynIO*);
    int (*handle_connected)(zcAsynIO*);
    int (*handle_error)(zcAsynIO*, int err);
    int (*handle_close)(zcAsynIO*);
    int (*handle_read)(zcAsynIO*);
    int (*handle_wrote)(zcAsynIO*);
    int (*handle_read_timeout)(zcAsynIO*);
    int (*handle_write_timeout)(zcAsynIO*);
}zcProtocol;

zcProtocol*	zc_protocol_new();
int			zc_protocol_init(zcProtocol*);

struct zc_asynio_t
{
	zcSocket	*sock;
	zcBuffer	*rbuf;
	zcBuffer	*wbuf;

    char        connected:1; // for connection created
    char        accepting:1; // flag for server listen
	char		close:1;
	char		ssl:1;
    char        protocol_free:1; // protocol is need free?
	char		w_init:1; // write event init complete
	char		rbuf_free:1; // rbuf need free
	char		wbuf_free:1; // wbuf need free

    int         read_timeout;
    int         write_timeout;
    int         conn_timeout;

	void		*data; // user defined data
	//zcCallChain *calls;

#ifdef ASYNC_ONE_WATCHER
    ev_io		watcher;
#else
    ev_io		r;
    ev_io		w;
#endif
	ev_timer	timer;
	ev_timer	rtimer;
	ev_timer	wtimer;
	struct ev_loop *loop;

	zcProtocol	p;

	int			_read_bytes;
	char*		_read_until;
	zcFuncReadx _read_callback;
};

zcAsynIO*	zc_asynio_new(zcSocket *sock, zcProtocol *p, struct ev_loop *loop, int rbufsize, int wbufsize);
zcAsynIO*	zc_asynio_new_buf(zcSocket *sock, zcProtocol *p, struct ev_loop *loop, zcBuffer *rbuf, zcBuffer *wbuf);
void		zc_asynio_delete(void*);
void        zc_asynio_delete_delay(void*);
#define     zc_asynio_safedel(x)  do{zc_asynio_delete(x);x=NULL;}while(0)
zcAsynIO*	zc_asynio_new_accepted(zcSocket *sock, zcAsynIO *pconn);
zcAsynIO*   zc_asynio_new_tcp_client(const char *host, int port, int timeout, zcProtocol *p,
				struct ev_loop *loop, int rbufsize, int wbufsize);
zcAsynIO*   zc_asynio_new_ssl_client(const char *host, int port, int timeout, zcProtocol *p,
				struct ev_loop *loop, int rbufsize, int wbufsize);
zcAsynIO*   zc_asynio_new_udp_client(const char *host, int port, int timeout, zcProtocol *p, 
				struct ev_loop *loop, int rbufsize, int wbufsize);

zcAsynIO*   zc_asynio_new_tcp_server(const char *host, int port, int timeout, zcProtocol *p,
				struct ev_loop *loop, int rbufsize, int wbufsize);
#define		zc_asynio_new_tcp_server_simple(host,port,timeout,loop,rbufsize,wbufsize)	zc_asynio_new_tcp_server(host,port,timeout,NULL,loop,rbufsize,wbufsize)
zcAsynIO*   zc_asynio_new_udp_server(const char *host, int port, int timeout, zcProtocol *p,
				struct ev_loop *loop, int rbufsize, int wbufsize);
void        zc_asynio_set_protocol(zcAsynIO *conn, zcProtocol *p);
void		zc_asynio_copy(zcAsynIO *conn, zcAsynIO *fromconn);
int			zc_asynio_connect(zcAsynIO *conn, const char *host, int port);
int			zc_asynio_call_later(zcAsynIO *conn, int after, int repeat, int (*callback)(zcAsynIO*, void*), void *data);
int			zc_asynio_read_bytes(zcAsynIO *conn, int len, zcFuncReadx f);
int			zc_asynio_read_until(zcAsynIO *conn, char*, zcFuncReadx f); 
void		zc_asynio_write_start(zcAsynIO *conn);

#endif

#endif
