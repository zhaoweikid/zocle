#ifndef ZOCLE_NET_COMMAND_H
#define ZOCLE_NET_COMMAND_H

#include <stdio.h>
#include <zocle/net/sockets.h>
#include <zocle/net/socketio.h>
#include <zocle/str/buffer.h>

#define ZC_CMD_TERM_SIZE	1
#define ZC_CMD_TERM_LINE	2

typedef struct
{
	zcSocket	*sock;	
	zcSocketIO	*rio;
	zcBuffer	*w;
	zcBuffer	*r;  // for line terminator
	int			headsize; // size of header
	int			reconn;  // try reconnect?
	int			term_type;
	int			bufsize;
	int			conn_interval;
}zcCmd;

zcCmd*	zc_cmd_new(zcSocket *sock, int bufsize, int reconn);
void	zc_cmd_delete(void *);
void	zc_cmd_term_size(zcCmd *c, int size);
void	zc_cmd_term_line(zcCmd *c);
zcBuffer* zc_cmd_rbuf(zcCmd *c);
zcBuffer* zc_cmd_wbuf(zcCmd *c);
int		zc_cmd_req(zcCmd *c);
int		zc_cmd_resp(zcCmd *c);

#endif
