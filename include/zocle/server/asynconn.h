#ifdef ZOCLE_WITH_LIBEV

#ifndef ZOCLE_SERVER_ASYNCONN_H
#define ZOCLE_SERVER_ASYNCONN_H

#include <zocle/net/sockets.h>
#include <zocle/str/buffer.h>
#include <ev.h>

struct zc_asynconn_t;
typedef struct zc_asynconn_t zcAsynConn;

struct zc_callchain_t;
typedef struct zc_callchain_t zcCallChain;
typedef int (*zcFuncCallBack)(zcAsynConn*, zcCallChain *chain, int ret);

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
int			 zc_callchain_call(zcCallChain*, zcAsynConn*, int);
#define		 zc_callchain_reset(x) x->cur=x->items
#define      zc_callchain_safedel(x)    do{zc_callchain_delete(x);x=NULL;}while(0)

struct zc_protocol_t
{

}zcProtocol;

struct zc_asynconn_t
{
	zcSocket	*sock;
	zcBuffer	*rbuf;
	zcBuffer	*wbuf;

    char        connected:1;
    char        accepting:1; // for server
	char		close:1;
	char		ssl:1;

    int         read_timeout;
    int         write_timeout;
    int         conn_timeout;

	void		*data; // user defined data
	zcCallChain *calls;

#ifdef ASYNC_ONE_WATCHER
    ev_io		watcher;
#else
    ev_io		r;
    ev_io		w;
#endif
	ev_timer	timer;
	struct ev_loop *loop;

    int (*handle_accept)(zcAsynConn*);
    int (*handle_connected)(zcAsynConn*);
    int (*handle_error)(zcAsynConn*, int err);
    int (*handle_close)(zcAsynConn*);
    int (*handle_read)(zcAsynConn*, zcBuffer *);
    int (*handle_ready)(zcAsynConn*, char *data, int datalen);
    int (*handle_wrote)(zcAsynConn*);
    int (*handle_read_timeout)(zcAsynConn*);
    int (*handle_write_timeout)(zcAsynConn*);
};

zcAsynConn*	zc_asynconn_new(zcSocket *sock, struct ev_loop *loop, int rbufsize, int wbufsize);
void		zc_asynconn_delete(void*);
#define     zc_asynconn_safedel(x)  do{zc_asynconn_delete(x);x=NULL;}while(0)
zcAsynConn* zc_asynconn_new_tcp_client(const char *host, int port, int timeout, 
				struct ev_loop *loop, int rbufsize, int wbufsize);
zcAsynConn* zc_asynconn_new_ssl_client(const char *host, int port, int timeout, 
				struct ev_loop *loop, int rbufsize, int wbufsize);
zcAsynConn* zc_asynconn_new_udp_client(const char *host, int port, int timeout, 
				struct ev_loop *loop, int rbufsize, int wbufsize);

zcAsynConn* zc_asynconn_new_tcp_server(const char *host, int port, int timeout, 
				struct ev_loop *loop, int rbufsize, int wbufsize);
zcAsynConn* zc_asynconn_new_udp_server(const char *host, int port, int timeout, 
				struct ev_loop *loop, int rbufsize, int wbufsize);

void		zc_asynconn_copy(zcAsynConn *conn, zcAsynConn *fromconn);
int			zc_asynconn_connect(zcAsynConn *conn, const char *host, int port);
int			zc_asynconn_call_later(zcAsynConn *conn, int after, int repeat, int (*callback)(zcAsynConn*, void*), void *data);
void		zc_asynconn_switch_read(zcAsynConn*);
#ifdef  ASYNC_ONE_WATCHER
void		zc_asynconn_switch_write(zcAsynConn*);
#else
void		zc_asynconn_start_write(zcAsynConn*);
#endif

#endif

#endif
