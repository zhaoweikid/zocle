#include <zocle/server/channel.h>
#include <zocle/base/defines.h>
#include <zocle/mem/alloc.h>
#include <unistd.h>
#include <zocle/net/sockets.h>
#include <zocle/log/logfile.h>

zcChannel* 
zc_channel_new(int size)
{
    zcChannel *ch = zc_calloct(zcChannel);
    
    int ret = zc_socket_pair_tcp(&ch->sender, &ch->receiver);
    if (ret < 0) {
        ZCWARN("socket pair create error: %d", ret);
        zc_free(ch);
        return NULL;
    }

    if (size > 0) {
        ch->sendq = zc_queue_new(size);
        if (ch->sendq == NULL) {
            zc_channel_delete(ch);
            return NULL;
        }
        ch->recvq = zc_queue_new(size);
        if (ch->recvq == NULL) {
            zc_channel_delete(ch);
            return NULL;
        }
    }

    return ch;
}


void
zc_channel_delete(void *x)
{
    zcChannel *ch = (zcChannel*)x;
    zc_socket_delete(ch->sender);
    zc_socket_delete(ch->receiver);

    if (ch->sendq) {
        zc_queue_delete(ch->sendq);
    }
    if (ch->recvq) {
        zc_queue_delete(ch->recvq);
    }
    zc_free(x);
}

int 
zc_channel_send(zcChannel *ch, void *data, int timeout)
{
    int ret;
    if (ch->sendq) {
        ret = zc_queue_put(ch->sendq, data, true, timeout);
        if (ret != ZC_OK) {
            return ret;
        }
        ret = zc_socket_sendn(ch->sender, "1", 1);
        if (ret != 1) {
            ZCWARN("sender send error: %d", ret);
            return ret;
        }
        return ZC_OK;
    }

    ret = zc_socket_sendn(ch->sender, data, sizeof(void*));
    if (ret != sizeof(void*)) {
        ZCWARN("sender send error: %d", ret);
        return ZC_ERR;
    }
    return ZC_OK;
}

void*
zc_channel_recv(zcChannel *ch, void *defv, int timeout)
{
    int ret;
    char buf[10];
    
    ch->receiver->timeout = timeout;
    if (ch->recvq) {
        ret = zc_socket_recvn(ch->receiver, buf, 1);
        if (ret != 1) {
            ZCWARN("recv error! %d", ret);
            return defv;
        }
        return zc_queue_get(ch->recvq, true, 0, NULL);
    }
    void *data = NULL;
    /*ret = zc_socket_recvn(ch->receiver, &data, sizeof(void*));
    if (ret != sizeof(void*)) {
        ZCWARN("recvn error: %d", ret);
        return defv;
    }*/

    return data;
}




