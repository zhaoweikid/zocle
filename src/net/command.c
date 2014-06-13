#include <zocle/net/command.h>
#include <zocle/net/sockets.h>
#include <zocle/log/logfile.h>
#include <zocle/mem/alloc.h>
#include <unistd.h>

static int
zc_cmd_head_data2size(zcCmd *c)
{
    uint64_t size = 0;
    switch(c->headsize) {
    case 1: {// char
        uint8_t v;    
        memcpy(&v, c->rio->rbuf->data, c->headsize);
        size = v;
        break;
    }
    case 2: {// short
        uint16_t v;    
        memcpy(&v, c->rio->rbuf->data, c->headsize);
        size = v;
        break;
    }
    case 4: {// int
        uint32_t v;
        memcpy(&v, c->rio->rbuf->data, c->headsize);
        size = v;
        break;
    }
    case 8: {// long long
        uint64_t v;
        memcpy(&v, c->rio->rbuf->data, c->headsize);
        size = v;
        break;
    }
    }
    return size;
}

static int
zc_cmd_head_size2data(zcCmd *c, char *buf)
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

zcCmd*  
zc_cmd_new(zcSocket *sock, int bufsize, int reconn)
{
    zcCmd *cmd;

    cmd = zc_malloc(sizeof(zcCmd));
    memset(cmd, 0, sizeof(zcCmd));
  
    if (bufsize < 4096)
        bufsize = 4096;

    cmd->sock = sock;
    cmd->bufsize = bufsize;
    cmd->rio = zc_sockio_new(sock, bufsize); //zc_buffer_new(4096);
    cmd->r = NULL;
    cmd->w = zc_buffer_new(bufsize);
    //cmd->headsize = 0;
    cmd->reconn   = reconn;
    cmd->conn_interval = 1000000;

    zc_cmd_term_line(cmd);

    return cmd;
}

void    
zc_cmd_delete(void *c)
{
    zcCmd *cmd = (zcCmd*)c;

    zc_sockio_delete(cmd->rio);
    zc_buffer_delete(cmd->w);
    if (cmd->term_type == ZC_CMD_TERM_LINE) {
        if (cmd->r)
            zc_buffer_delete(cmd->r);
    }
    zc_free(cmd);
}

void    
zc_cmd_term_size(zcCmd *c, int size)
{
    c->term_type = ZC_CMD_TERM_SIZE;
    c->headsize = size;
}

void    
zc_cmd_term_line(zcCmd *c)
{
    if (c->term_type == ZC_CMD_TERM_LINE)
        return;
    c->term_type = ZC_CMD_TERM_LINE;
    if (c->r) {
        zc_buffer_delete(c->r);
    }
    c->r = zc_buffer_new(c->bufsize);
}


zcBuffer* 
zc_cmd_rbuf(zcCmd *c)
{
    return c->r;
}

zcBuffer* 
zc_cmd_wbuf(zcCmd *c)
{
    return c->w;
}

static int
zc_cmd_reconnect(zcCmd *c)
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
zc_cmd_req(zcCmd *c)
{
    int ret;
 
    if (zc_socket_peek(c->sock) == ZC_FALSE) {
        if (zc_cmd_reconnect(c) != ZC_OK) {
            ZCWARN("connection error\n");
            return ZC_ERR;
        }
    }
    if (c->term_type == ZC_CMD_TERM_SIZE) {
        char buf[8] = {0};
        zc_cmd_head_size2data(c, buf);
        ret = zc_socket_sendn(c->sock, buf, c->headsize);
        if (ret < 0) {
            ZCWARN("send head %d error\n", c->w->end);
            return ZC_ERR;
        }
        //ZCINFO("send head:%d\n", ret);
    }

    ret = zc_socket_sendn(c->sock, c->w->data, c->w->end);
    if (ret <= 0) {
        ZCWARN("send len:%d error\n", c->w->end);
        return ZC_ERR;
    }
    //ZCINFO("send body:%d\n", ret);
    if (c->term_type == ZC_CMD_TERM_LINE) {
        ret = zc_socket_sendn(c->sock, "\r\n", 2);
        if (ret <= 0) {
            ZCWARN("send line error\n");
            return ZC_ERR;
        }
        //ZCINFO("send endline:%d\n", ret);
    }
    return ZC_OK;
}

int 
zc_cmd_resp(zcCmd *c) 
{
    int ret;
    if (c->term_type == ZC_CMD_TERM_SIZE) {
        zc_buffer_reset(c->rio->rbuf);
        ret = zc_sockio_read_nocopy(c->rio, c->headsize);
        if (ret < 0) {
            return ret;
        }

        uint64_t size = zc_cmd_head_data2size(c);
        if (size == 0) {
            ZCERROR("read head size error:%llu\n", (unsigned long long)size);
        }
        //ZCINFO("try read body size:%llu\n", size);

        zc_buffer_reset(c->rio->rbuf);
        ret = zc_sockio_read_nocopy(c->rio, size);
        if (ret < 0) {
            ZCWARN("read %llu error:%d\n", (unsigned long long)size, ret);
            return ret;
        }
        
        c->r = c->rio->rbuf;
        //ZCINFO("read body size:%u, %s\n", c->rio->rbuf->len, c->rio->rbuf->data);
        return zc_buffer_len(c->rio->rbuf);
    }else if (c->term_type == ZC_CMD_TERM_LINE) {
        zc_buffer_reset(c->r);
        ret = zc_sockio_readline(c->rio, c->r->data, sizeof(c->r->size));
        if (ret <= 0) {
            ZCWARN("readline error:%d\n", ret);
            return ret;
        }
        c->r->end = ret;
        return ret;
    }
        
    return ZC_ERR;
}


