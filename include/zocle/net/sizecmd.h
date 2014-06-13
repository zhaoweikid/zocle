#ifndef ZOCLE_NET_SIZECMD_H
#define ZOCLE_NET_SIZECMD_H

#include <stdio.h>
#include <zocle/net/sockets.h>
#include <zocle/net/socketio.h>
#include <zocle/str/buffer.h>

struct zc_sizecmd_t;
typedef struct zc_sizecmd_t zcSizeCmd;

struct zc_sizecmd_t
{
	zcSocket	*sock;	
	zcBuffer	*w;
	zcBuffer	*r;  // for line terminator
    int         headsize;
	int			rbufsize;
	int			wbufsize;
	int			reconn;  // try reconnect?
	int			conn_interval;

    int (*head_size2data)(zcSizeCmd *c, char *buf);
    int (*head_data2size)(zcSizeCmd *c);
};

zcSizeCmd*	zc_sizecmd_new(zcSocket *sock, int rbufsize, int wbufsize, int reconn);
void	    zc_sizecmd_delete(void *);
int		    zc_sizecmd_req(zcSizeCmd *c);
int		    zc_sizecmd_resp(zcSizeCmd *c);
int         zc_sizecmd_cmd(zcSizeCmd *c);

#define     zc_sizecmd_rbuf(c)  c->r
#define     zc_sizecmd_wbuf(c)  c->w

#endif
