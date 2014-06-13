#ifndef ZOCLE_SERVER_CHANNEL_H
#define ZOCLE_SERVER_CHANNEL_H

#include <stdio.h>
#include <zocle/net/sockets.h>
#include <zocle/ds/queue.h>

typedef struct zc_channel_t
{
	zcSocket *sender;
	zcSocket *receiver;
	
	zcQueue *sendq;
	zcQueue *recvq;

}zcChannel;

zcChannel* zc_channel_new(int size);
void	   zc_channel_delete(void*);
int		   zc_channel_send(zcChannel*, void*, int timeout);
void*	   zc_channel_recv(zcChannel*, void *defv, int timeout);


#endif
