#ifndef ZOCLE_NET_LINECMD_H
#define ZOCLE_NET_LINECMD_H

#include <stdio.h>
#include <zocle/net/sockets.h>
#include <zocle/net/socketio.h>
#include <zocle/str/buffer.h>

struct zc_linecmd_t;
typedef struct zc_linecmd_t zcLineCmd;

struct zc_linecmd_t
{
	zcSocket	*sock;	
	zcSocketIO	*rio;
	zcBuffer	*w;
	zcBuffer	*r;  // for line terminator
	int			reconn;  // try reconnect?
	int			rbufsize;
	int			wbufsize;
	int			conn_interval;
};

zcLineCmd*	zc_linecmd_new(zcSocket *sock, int rbufsize, int wbufsize, int reconn);
void	    zc_linecmd_delete(void *);
int		    zc_linecmd_req(zcLineCmd *c);
int		    zc_linecmd_resp(zcLineCmd *c);

#define zc_linecmd_rbuf(c)  c->r
#define zc_linecmd_wbuf(c)  c->w

#endif
