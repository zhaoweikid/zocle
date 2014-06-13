#include <zocle/net/linecmd.h>
#include <zocle/net/sockets.h>
#include <zocle/log/logfile.h>
#include <zocle/mem/alloc.h>

zcLineCmd*  
zc_linecmd_new(zcSocket *sock, int rbufsize, int wbufsize, int reconn)
{
    zcLineCmd *cmd;

    cmd = zc_malloc(sizeof(zcLineCmd));
    memset(cmd, 0, sizeof(zcLineCmd));
  
    if (rbufsize < 4096)
        rbufsize = 4096;
    if (wbufsize < 4096)
        wbufsize = 4096;

    cmd->sock = sock;
    cmd->rbufsize = rbufsize;
    cmd->wbufsize = wbufsize;
    cmd->rio = zc_sockio_new(sock, rbufsize); //zc_buffer_new(4096);
    cmd->r = zc_buffer_new(rbufsize);
    cmd->w = zc_buffer_new(wbufsize);
    cmd->reconn   = reconn;
    cmd->conn_interval = 1000000;

    return cmd;
}

void    
zc_linecmd_delete(void *c)
{
    zcLineCmd *cmd = (zcLineCmd*)c;

    zc_sockio_delete(cmd->rio);
    zc_buffer_delete(cmd->w);
    zc_buffer_delete(cmd->r);
    zc_free(cmd);
}

static int
zc_linecmd_reconnect(zcLineCmd *c)
{
    int tryn = c->reconn;
    while (tryn > 0) {
        if (zc_socket_reconnect(c->sock) < 0) {
            if (tryn <= 0)
                return ZC_ERR;
            tryn--;
            usleep(c->conn_interval);
            ZCNOTICE("reconnect %d", tryn);
            continue;
        }
        break;
    }
    return ZC_OK;
}

int
zc_linecmd_req(zcLineCmd *c)
{
    int ret;
 
    if (zc_socket_peek(c->sock) == ZC_FALSE) {
        if (zc_linecmd_reconnect(c) != ZC_OK) {
            ZCWARN("connection error\n");
            return ZC_ERR;
        }
    }
    ret = zc_socket_sendn(c->sock, c->w->data, c->w->end);
    if (ret <= 0) {
        ZCWARN("send len:%d error\n", c->w->end);
        return ZC_ERR;
    }
    //ZCINFO("send body:%d\n", ret);
    ret = zc_socket_sendn(c->sock, "\r\n", 2);
    if (ret <= 0) {
        ZCWARN("send line error\n");
        return ZC_ERR;
    }
    //ZCINFO("send endline:%d\n", ret);
    return ZC_OK;
}

int 
zc_linecmd_resp(zcLineCmd *c) 
{
    int ret;
    zc_buffer_reset(c->r);
    ret = zc_sockio_readline(c->rio, c->r->data, sizeof(c->r->size));
    if (ret <= 0) {
        ZCWARN("readline error:%d\n", ret);
        return ret;
    }
    c->r->end = ret;
    return ret;
}


