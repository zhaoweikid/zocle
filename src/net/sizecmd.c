#include <zocle/net/sizecmd.h>
#include <zocle/net/sockets.h>
#include <zocle/log/logfile.h>
#include <zocle/mem/alloc.h>

static int
zc_sizecmd_head_data2size(zcSizeCmd *c)
{
    uint64_t size = 0;
    switch(c->headsize) {
    case 1: {// char
        uint8_t v;    
        memcpy(&v, c->r->data, c->headsize);
        size = v;
        break;
    }
    case 2: {// short
        uint16_t v;    
        memcpy(&v, c->r->data, c->headsize);
        size = v;
        break;
    }
    case 4: {// int
        uint32_t v;
        memcpy(&v, c->r->data, c->headsize);
        size = v;
        break;
    }
    case 8: {// long long
        uint64_t v;
        memcpy(&v, c->r->data, c->headsize);
        size = v;
        break;
    }
    }
    return size;
}

static int
zc_sizecmd_head_size2data(zcSizeCmd *c, char *buf)
{
    switch(c->headsize) {
        case 1: {// char
            uint8_t v = c->w->end;
            memcpy(buf, &v, sizeof(v));
            break;
        }
        case 2: {// short
            uint16_t v = c->w->end;    
            memcpy(buf, &v, sizeof(v));
            break;
        }
        case 4: {// int
            uint32_t v = c->w->end;
            memcpy(buf, &v, sizeof(v));
            break;
        }
        case 8: {// long long
            uint64_t v = c->w->end;
            memcpy(buf, &v, sizeof(v));
            break;
        }
    }
    return c->headsize;
}

zcSizeCmd*  
zc_sizecmd_new(zcSocket *sock, int rbufsize, int wbufsize, int reconn)
{
    zcSizeCmd *cmd;

    cmd = zc_malloc(sizeof(zcSizeCmd));
    memset(cmd, 0, sizeof(zcSizeCmd));
  
    if (rbufsize < 4096)
        rbufsize = 4096;
    if (wbufsize < 4096)
        wbufsize = 4096;

    cmd->sock = sock;
    cmd->rbufsize = rbufsize;
    cmd->wbufsize = wbufsize;
    cmd->r = zc_buffer_new(rbufsize);
    cmd->w = zc_buffer_new(wbufsize);
    cmd->reconn = reconn;
    cmd->conn_interval = 1000000;

    cmd->head_data2size = zc_sizecmd_head_data2size;
    cmd->head_size2data = zc_sizecmd_head_size2data;

    return cmd;
}

void    
zc_sizecmd_delete(void *c)
{
    zcSizeCmd *cmd = (zcSizeCmd*)c;

    zc_buffer_delete(cmd->r);
    zc_buffer_delete(cmd->w);
    
    zc_free(cmd);
}

void    
zc_sizecmd_term(zcSizeCmd *c, int headsize,  
        int (*head_size2data)(zcSizeCmd*, char *),  
        int (*head_data2size)(zcSizeCmd*))
{
    c->headsize = headsize;
    c->head_size2data = head_size2data;
    c->head_data2size = head_data2size;
}


static int
zc_sizecmd_reconnect(zcSizeCmd *c)
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
zc_sizecmd_req(zcSizeCmd *c)
{
    int ret;
 
    if (zc_socket_peek(c->sock) == ZC_FALSE) {
        if (zc_sizecmd_reconnect(c) != ZC_OK) {
            ZCWARN("connection error\n");
            return ZC_ERR;
        }
    }
    char buf[16] = {0};
    int headsize = c->head_size2data(c, buf);
    if (headsize < 0) {
        ZCWARN("head_size2data error:%d", headsize);
        return ZC_ERR;
    }
    ret = zc_socket_sendn(c->sock, buf, headsize);
    if (ret < 0) {
        ZCWARN("send head %d error\n", c->w->end);
        return ZC_ERR;
    }
    //ZCINFO("send head:%d\n", ret);
    ret = zc_socket_sendn(c->sock, c->w->data, c->w->end);
    if (ret <= 0) {
        ZCWARN("send len:%d error\n", c->w->end);
        return ZC_ERR;
    }
    return ZC_OK;
}

int 
zc_sizecmd_resp(zcSizeCmd *c) 
{
    int ret;
    zc_buffer_reset(c->r);
    if (c->headsize > 0) {
        ret = zc_socket_recvn(c->sock, c->r->data, c->headsize); 
        if (ret < 0) {
            return ret;
        }
    }

    int size = c->head_data2size(c);
    if (size == 0) {
        ZCERROR("read head size error:%llu\n", (unsigned long long)size);
        return ZC_ERR;
    }
    //ZCINFO("try read body size:%llu\n", size);

    zc_buffer_reset(c->r);
    ret = zc_socket_recvn(c->sock, c->r->data, size); 
    if (ret < 0) {
        ZCWARN("read %llu error:%d\n", (unsigned long long)size, ret);
        return ret;
    }
    c->r->end = ret;
    //ZCINFO("read body size:%u, %s\n", c->rio->r->len, c->rio->r->data);
    return zc_buffer_len(c->r);
}


