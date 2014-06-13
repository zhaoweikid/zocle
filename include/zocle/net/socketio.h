#ifndef ZOCLE_NET_SOCKETIO_H
#define ZOCLE_NET_SOCKETIO_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <zocle/net/sockets.h>
#include <zocle/str/string.h>
#include <zocle/str/cstring.h>
#include <zocle/str/buffer.h>
#include <zocle/ds/list.h>

typedef struct zc_socketio_t
{
    zcSocket	*sock;
	zcBuffer	*rbuf;
}zcSocketIO;

typedef int (*zcLineTerminatorFunc)(char *line, int len);

zcSocketIO* zc_sockio_new (zcSocket *s, int size);
void        zc_sockio_delete (void*);
void        zc_sockio_delete4_ (void*, void*, const char*, int);

int         zc_sockio_read (zcSocketIO*, char *buf, int len);
int         zc_sockio_read_nocopy (zcSocketIO*, int len);
int         zc_sockio_readn (zcSocketIO *, char *buf, int len);
int         zc_sockio_readline (zcSocketIO*, char *buf, int len);
zcString*   zc_sockio_readline_str (zcSocketIO*, zcString*);
int			zc_sockio_readline_cstr (zcSocketIO *s, zcCString *str);
zcList*     zc_sockio_readlines (zcSocketIO*, int num);
zcList*     zc_sockio_readlines_endline (zcSocketIO*, char *endline);
zcList*     zc_sockio_readlines_custom (zcSocketIO*, zcLineTerminatorFunc check);

int         zc_sockio_write (zcSocketIO*, char *buf, int len);
int         zc_sockio_writen (zcSocketIO*, char *buf, int len);
int         zc_sockio_write_list (zcSocketIO*, zcList *s);

#define     zc_sockio_delete4(x) \
				do{zc_sockio_delete4_(x,NULL,__FILE__,__LINE__);x=NULL;}while(0)

#endif

