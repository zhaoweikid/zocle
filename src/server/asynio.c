#ifdef ZOCLE_WITH_LIBEV

#include <zocle/server/asynio.h>
#include <zocle/log/logfile.h>
#include <zocle/base/defines.h>
#include <zocle/mem/alloc.h>
#include <zocle/str/buffer.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>

#ifndef EALREADY
#define EALREADY  103
#endif

#ifndef EWOULDBLOCK
#define EWOULDBLOCK 140
#endif

#ifndef EINPROGRESS
#define EINPROGRESS 112
#endif

#define ZC_ONE_ACCEPT_MAX   1
#define EPSINON   (float)0.00001


static const char *ZC_ASYNIO_READ_CLOSE = "<close>";


static void zc_asynio_write_stop(zcAsynIO *conn);

zcCallChain* 
zc_callchain_new()
{
    zcCallChain *cc = zc_calloct(zcCallChain);
    return cc;
}

void		 
zc_callchain_delete(void *x)
{
    zc_free(x);
}

int
zc_callchain_append(zcCallChain *cc, zcFuncCallBack f, zcFuncCallBack errf, void *data)
{
    zcCallItem *ci = zc_calloct(zcCallItem);
    ci->func = f;
    ci->errfunc = errf;
    ci->data = data;
    
    zcCallItem *last = cc->items;     
    if (last) {
        while (last->next) {
            last = (zcCallItem*)last->next;
        }
        last->next = ci;
    }else{
        cc->items = ci;
        cc->cur = ci;
    }

    return ZC_OK;
}

int
zc_callchain_prepend(zcCallChain *cc, zcFuncCallBack f, zcFuncCallBack errf, void *data)
{
    zcCallItem *ci = zc_calloct(zcCallItem);
    ci->func = f;
    ci->errfunc = errf;
    ci->data = data;
    
    ci->next = cc->items; 
    cc->items = ci;
    cc->cur = ci;

    return ZC_OK;
}

int	
zc_callchain_call(zcCallChain *cc, zcAsynIO *conn, int ret)
{
    if (cc->cur == NULL) {
        return ZC_ERR;
    }
    int err;
    if (ret < 0) {
        err = cc->cur->func(conn, cc->cur->data, ret);
    }else{
        err = cc->cur->errfunc(conn, cc->cur->data, ret);
    }
    cc->cur = (zcCallItem*)cc->cur->next;
    return err;
}

/*static int
zc_asynio_set_closed(zcAsynIO *conn)
{
    conn->connected = ZC_FALSE;
    return ZC_OK;
}*/



// accept only one socket
zcSocket*
zc_asynio_handle_accept_one(zcAsynIO *conn)
{
    zcSocket *newsock = zc_socket_accept(conn->sock);
    if (NULL == newsock) {
        return NULL;
    }
    newsock->timeout = conn->sock->timeout;
    zcAsynIO *newconn = zc_asynio_new(newsock, &conn->p, conn->loop, conn->rbuf->size, conn->wbuf->size);
    newconn->accepting = ZC_FALSE;
    newconn->connected = ZC_FALSE;
    zc_asynio_copy(newconn, conn);

    //ev_io_init(&newconn->w, zc_asynio_ev_write, newconn->sock->fd, EV_WRITE);

    return newsock;
}


// accept event
int
zc_asynio_handle_accept(zcAsynIO *conn)
{
    int i;
    for (i=0; i<ZC_ONE_ACCEPT_MAX; i++) {
        zcSocket *newsock = zc_asynio_handle_accept_one(conn);
        if (NULL == newsock)
            break;
    }
    return ZC_OK;
}

// connected event
int
zc_asynio_handle_connected(zcAsynIO *conn)
{
    ZCNOTE("connected");
    if (conn->ssl) {
        // async ssl?
        // zc_socket_client_ssl(conn->sock, NULL, NULL);
    }
    return ZC_OK;
}
/*int
zc_asynio_handle_connlost(zcAsynIO *conn)
{
    ZCNOTE("connection lost");
    return ZC_OK;
}
int
zc_asynio_handle_connfailed(zcAsynIO *conn)
{
    ZCNOTE("connection failed");
    return ZC_OK;
}*/


// error event
int
zc_asynio_handle_error(zcAsynIO *conn, int err)
{
    ZCNOTE("connection error! %d", err);
    return ZC_OK;
}


// close event
int
zc_asynio_handle_close(zcAsynIO *conn)
{
    ZCNOTE("connection %s:%d close (%p)", conn->sock->remote.ip, conn->sock->remote.port, conn);
    return ZC_OK;
}

// read event
int
zc_asynio_handle_read(zcAsynIO *conn)
{
    zcBuffer *buf = conn->rbuf;
    int rlen = 0;

    ZCDEBUG("read_bytes:%d, read_until:%s, rbuf:%d", conn->_read_bytes, conn->_read_until, 
                zc_buffer_used(buf));

    char *data = zc_buffer_data(buf);
    if (conn->_read_bytes > 0 && zc_buffer_used(buf) >= conn->_read_bytes) { // read_bytes
        ZCDEBUG("read bytes call");
        rlen = conn->_read_bytes;
        conn->_read_callback(conn, data, rlen);
        //conn->_read_bytes = 0;
    }else if (conn->_read_until != NULL) { // read to a position
        if (conn->_read_until == ZC_ASYNIO_READ_CLOSE) { // read to end
            if (conn->close) {
                ZCDEBUG("read util call");
                rlen = zc_buffer_used(buf);
                conn->_read_callback(conn, data, rlen);
                //conn->_read_until = NULL;
            }
        }else{
            char *pos = strstr(data, conn->_read_until);
            if (NULL != pos) { // found
                ZCDEBUG("read util call");
                rlen = pos-data+strlen(conn->_read_until);
                conn->_read_callback(conn, data, rlen); 
                //conn->_read_until = NULL;
            }   
        }
    }
    if (rlen == 0) {
        return ZC_AGAIN;
    }else{
        return rlen;
    }
}

// wrote event
int
zc_asynio_handle_wrote(zcAsynIO *conn)
{
    //ZCNOTE("data wrote");
    return 0;
}

// read timeout event
int
zc_asynio_handle_read_timeout(zcAsynIO *conn)
{
    ZCNOTE("read timeout, close conn:%p", conn);
    conn->p.handle_close(conn);
    //zc_asynio_delete(conn);
    //zc_asynio_safedel(conn);
    zc_asynio_delete_delay(conn);
    return ZC_OK;
}

// write timeout event
int
zc_asynio_handle_write_timeout(zcAsynIO *conn)
{
    ZCNOTE("write timeout");
    return ZC_OK;
}


zcProtocol* 
zc_protocol_new()
{
    zcProtocol *p = zc_calloct(zcProtocol);
    if (zc_protocol_init(p) != ZC_OK) {
        zc_free(p);
        return NULL;
    }
    return p;
}

int
zc_protocol_init(zcProtocol *p)
{
    p->handle_accept     = zc_asynio_handle_accept;;
    p->handle_connected  = zc_asynio_handle_connected;
    p->handle_error      = zc_asynio_handle_error;
    p->handle_close      = zc_asynio_handle_close;
    p->handle_read       = zc_asynio_handle_read;
    p->handle_wrote      = zc_asynio_handle_wrote;
    p->handle_read_timeout  = zc_asynio_handle_read_timeout;
    p->handle_write_timeout = zc_asynio_handle_write_timeout;

    return ZC_OK;
}

static void zc_asynio_read_timer_reset(zcAsynIO *conn);

#define ZC_ONE_READ_MAX 8192*2

// read data to buffer
static int
zc_asynio_read_buffer(zcAsynIO *conn, zcBuffer *buf)
{
    int rsize = 0;
    while (zc_buffer_idle(buf) > 0) {
        int ret = zc_socket_recv(conn->sock, buf->data+buf->end, 
                zc_buffer_idle(buf)); 
        if (ret == 0 || (ret < 0 && ret != -EWOULDBLOCK && ret != -EAGAIN)) {
            ZCNOTE("recv error:%d, close conn", ret);
            if (ret == 0) { // read close
                conn->close = 1;
                if (zc_buffer_used(buf) > 0) {
                    conn->p.handle_read(conn);
                }
            }
            conn->p.handle_close(conn);
            //zc_asynio_safedel(conn);
            zc_asynio_delete_delay(conn);
            return ZC_ERR;
        }
        if (ret < 0) { // EWOULDBLOCK, EAGAIN
            break;
        }
        buf->end += ret;
        rsize += ret;
        if (rsize >= ZC_ONE_READ_MAX) // read max 8192, then break
            break;
    }
    return rsize;
}

// libev read callback
static void 
zc_asynio_ev_read(struct ev_loop *loop, ev_io *r, int events)
{
    zcAsynIO *conn = (zcAsynIO*)r->data; 

    char addr[32];
    sprintf(addr, "%s:%d ", conn->sock->remote.ip, conn->sock->remote.port);
    zc_log_set_prefix(_zc_log, addr);

    //ZCINFO("ok, read event, conn:%p\n", conn);
    int ret = 0;
    ev_timer_stop(loop, &conn->rtimer);
    // udp
    if (conn->sock->type == SOCK_DGRAM) {
        zcBuffer *rbuf = conn->rbuf;
        ret = zc_socket_recvfrom_self(conn->sock, zc_buffer_data(rbuf), zc_buffer_idle(rbuf), 0);
        if (ret <= 0) {
            ZCNOTE("recvfrom error:%d, close conn", ret);
            conn->p.handle_close(conn);
            //zc_asynio_safedel(conn);
            zc_asynio_delete_delay(conn);
            return;
        }
        rbuf->end += ret;
        ZCINFO("udp read:%d", ret);
        ret = conn->p.handle_read(conn);
        if (conn->rbuf_auto_compact) {
            rbuf->pos += ret;
            zc_buffer_compact(rbuf);
        }
        //zc_buffer_reset(rbuf);
        if (zc_buffer_used(conn->wbuf) > 0) {
            zc_asynio_write_start(conn);
        }
        zc_asynio_read_timer_reset(conn);
        return;
    }

    // tcp
    if (conn->accepting) { // do accept only, for listen socket
        conn->p.handle_accept(conn);
        return;
    }
    if (conn->connected == ZC_FALSE) {
        conn->p.handle_connected(conn);
        conn->connected = ZC_TRUE;
    }
    ZCDEBUG("try read data\n");
    zcBuffer *rbuf = conn->rbuf;
    int rsize = 0;
    ret = zc_asynio_read_buffer(conn, rbuf);
    ZCDEBUG("read data %d to rbuf", ret);
    if (ret < 0) //close
        return;
    rsize += ret;

    ZCDEBUG("buffer used:%d", zc_buffer_used(rbuf));
    while (zc_buffer_used(rbuf) > 0) {
        ZCDEBUG("handle read %d", zc_buffer_used(rbuf));
        ret = conn->p.handle_read(conn);
        ZCDEBUG("handle_read return:%d", ret);
        if (ret == ZC_AGAIN) // read again
            break;
        if (ret < 0) {
            ZCWARN("handle_read failed %d, break", ret);
            conn->p.handle_close(conn);
            //zc_asynio_safedel(conn);
            zc_asynio_delete_delay(conn);
            return;
        }
        if (conn->rbuf_auto_compact) {
            rbuf->pos += ret;
        }
    }
    if (conn->rbuf_auto_compact) {
        zc_buffer_compact(conn->rbuf);
    }

    if (zc_buffer_used(conn->wbuf) > 0) {
        zc_asynio_write_start(conn);
    }
    zc_asynio_read_timer_reset(conn);

    ZCDEBUG("<%s read event end>", addr);
    return;
}

// libev write callback
static void 
zc_asynio_ev_write(struct ev_loop *loop, ev_io *w, int events)
{
    //ZCINFO("ok, write event\n");
    zcAsynIO *conn = (zcAsynIO*)w->data; 

    char addr[32];
    sprintf(addr, "%s:%d ", conn->sock->remote.ip, conn->sock->remote.port);
    zc_log_set_prefix(_zc_log, addr);

    // stop write timeout
    ev_timer_stop(loop, &conn->wtimer);
    int ret;
    zcBuffer *wbuf = conn->wbuf;
    // udp
    if (conn->sock->type == SOCK_DGRAM) {
        ret = zc_socket_sendto_self(conn->sock, zc_buffer_data(wbuf), zc_buffer_used(wbuf), 0);
        if (ret != zc_buffer_used(wbuf)) {
            ZCWARN("sendto error:%d, wbuf:%d, close conn", ret, zc_buffer_used(wbuf));
            conn->p.handle_close(conn);
            //zc_asynio_safedel(conn);
            zc_asynio_delete_delay(conn);
            return;
        }
        ZCINFO("udp write:%d", ret);
        conn->p.handle_wrote(conn);
        zc_buffer_reset(wbuf);

        if (zc_buffer_used(conn->wbuf) == 0) {
            zc_asynio_write_stop(conn);
        }
        return;
    }

    // tcp
    // optimize?
    ret = zc_socket_peek(conn->sock);
    if (ret <= 0 && ret != -EAGAIN) {
        if (zc_socket_conn_lost(-1*ret)) {
            conn->p.handle_error(conn, ret);
        }
        ZCWARN("peek error:%d\n", ret);
        conn->p.handle_close(conn); 
        //zc_socket_delete(conn->sock);
        //zc_asynio_safedel(conn);
        zc_asynio_delete_delay(conn);
        return;
    }

    while (zc_buffer_used(wbuf) > 0) {
        //ZCINFO("write len:%d, pos:%d\n", zc_buffer_used(wbuf), wbuf->pos);
        ret = zc_socket_send(conn->sock, wbuf->data+wbuf->pos, zc_buffer_used(wbuf)); 
        if (ret < 0) {
            ZCWARN("send error:%d\n", ret);
            if (ret == -EAGAIN || ret == -EWOULDBLOCK) {
                ZCWARN("write EAGAIN, break");
                break;
            }
            conn->p.handle_close(conn);
            //zc_socket_delete(conn->sock);
            //zc_asynio_safedel(conn);
            zc_asynio_delete_delay(conn);
            return;
        }
        wbuf->pos += ret;
    }
    ZCINFO("after send, wbuf:%d", zc_buffer_used(conn->wbuf));
    if (zc_buffer_used(conn->wbuf) == 0) {
        if (conn->wbuf->next == NULL) {
            zc_buffer_reset(conn->wbuf);
            if (conn->p.handle_wrote(conn) == 0) { // handle_wrote return 0 means no new data, >0 have new data, not stop write
                zc_asynio_write_stop(conn);
            }
        }else{
            zcBuffer *mybuf = conn->wbuf;
            conn->wbuf = conn->wbuf->next;
            zc_buffer_delete(mybuf);
        }
    }
    //ZCINFO("write event end");
    return;
}

static void 
zc_asynio_read_ev_timeout(struct ev_loop *loop, ev_timer *r, int events)
{
    zcAsynIO *conn = (zcAsynIO*)r->data; 
    conn->p.handle_read_timeout(conn);
}

static void 
zc_asynio_write_ev_timeout(struct ev_loop *loop, ev_timer *r, int events)
{
    zcAsynIO *conn = (zcAsynIO*)r->data; 
    conn->p.handle_write_timeout(conn);
}

static void
zc_asynio_write_init(zcAsynIO *conn)
{
    ev_io_init(&conn->w, zc_asynio_ev_write, conn->sock->fd, EV_WRITE);
    conn->w.data = conn;
    
    if (conn->write_timeout > 0) {
        float tm = conn->write_timeout/1000.0;
        ev_timer_init(&conn->wtimer, zc_asynio_write_ev_timeout, tm, tm);
        conn->wtimer.data = conn;
    }
}


void
zc_asynio_write_start(zcAsynIO *conn)
{
    if (!conn->w_init) {
        zc_asynio_write_init(conn);
        conn->w_init = 1;
    }
    ev_io_start(conn->loop, &conn->w);

    if (conn->write_timeout > 0) {
        float tm = conn->write_timeout/1000.0;
        ev_timer_set(&conn->wtimer, tm, tm);
        //ev_timer_init(&conn->wtimer, zc_asynio_write_ev_timeout, tm, tm);
        //conn->wtimer.data = conn;
        ev_timer_start(conn->loop, &conn->wtimer);
    }
}

static void
zc_asynio_write_stop(zcAsynIO *conn)
{
    ev_io_stop(conn->loop, &conn->w);
    if (conn->write_timeout > 0) {
        ev_timer_stop(conn->loop, &conn->wtimer);
    }
}

static void
zc_asynio_read_timer_reset(zcAsynIO *conn)
{
    if (conn->read_timeout > 0) {
        float tm = conn->read_timeout/1000.0;
        ev_timer_set(&conn->rtimer, tm, tm);
        ev_timer_start(conn->loop, &conn->rtimer);
    }
}




zcAsynIO* 
zc_asynio_new(zcSocket *sock, zcProtocol *p, struct ev_loop *loop, int rbufsize, int wbufsize)
{
    zcAsynIO *conn = NULL;
    conn = zc_calloct(zcAsynIO);
    zc_socket_setblock(sock, ZC_FALSE);

    conn->rbuf = zc_buffer_new(rbufsize);
    conn->wbuf = zc_buffer_new(wbufsize);
    conn->rbuf_free = 1;
    conn->wbuf_free = 1;
    conn->rbuf_auto_compact = 1;

    /*conn->connected = ZC_FALSE;
    conn->accepting = ZC_FALSE;
    conn->close= ZC_FALSE;
    conn->ssl  = ZC_FALSE;*/
    conn->sock = sock;
    conn->loop = loop;

    if (p) {
        zc_asynio_set_protocol(conn, p);
    }else{ //default
        zc_protocol_init(&conn->p);
    }

    ev_io_init(&conn->r, zc_asynio_ev_read, conn->sock->fd, EV_READ);
    conn->r.data = conn;
    ev_io_start(conn->loop, &conn->r);
    
    int timeout = sock->timeout;
    conn->read_timeout = conn->write_timeout = conn->conn_timeout = timeout;
    //ZCINFO("timeout:%d\n", timeout);
    if (timeout > 0) {
        //ev_timer_init(&conn->timer, zc_asynio_ev_timeout, timeout, 1);
        float tm = timeout/1000.0;
        ev_timer_init(&conn->rtimer, zc_asynio_read_ev_timeout, tm, tm);
        conn->rtimer.data = conn;
        ev_timer_start(conn->loop, &conn->rtimer);
    }

    return conn;
}

zcAsynIO* 
zc_asynio_new_buf(zcSocket *sock, zcProtocol *p, struct ev_loop *loop, zcBuffer *rbuf, zcBuffer *wbuf)
{
    zcAsynIO *conn = NULL;
    conn = zc_calloct(zcAsynIO);
    zc_socket_setblock(sock, ZC_FALSE);

    conn->rbuf = rbuf;
    conn->wbuf = wbuf;
    conn->rbuf_free = 0;
    conn->wbuf_free = 0;
    conn->rbuf_auto_compact = 1;
    /*conn->connected = ZC_FALSE;
    conn->accepting = ZC_FALSE;
    conn->close= ZC_FALSE;
    conn->ssl  = ZC_FALSE;*/
    conn->sock = sock;
    conn->loop = loop;

    if (p) {
        zc_asynio_set_protocol(conn, p);
    }else{ //default
        zc_protocol_init(&conn->p);
    }

    ev_io_init(&conn->r, zc_asynio_ev_read, conn->sock->fd, EV_READ);
    conn->r.data = conn;
    ev_io_start(conn->loop, &conn->r);
    
    int timeout = sock->timeout;
    conn->read_timeout = conn->write_timeout = conn->conn_timeout = timeout;
    //ZCINFO("timeout:%d\n", timeout);
    if (timeout > 0) {
        //ev_timer_init(&conn->timer, zc_asynio_ev_timeout, timeout, 1);
        float tm = timeout/1000.0;
        ev_timer_init(&conn->rtimer, zc_asynio_read_ev_timeout, tm, tm);
        conn->rtimer.data = conn;
        ev_timer_start(conn->loop, &conn->rtimer);
    }

    return conn;
}

zcAsynIO* 
zc_asynio_new_accepted(zcSocket *sock, zcAsynIO *pconn)
{
    sock->timeout = pconn->sock->timeout;
    zcAsynIO *newconn = zc_asynio_new(sock, &pconn->p, pconn->loop, pconn->rbuf->size, pconn->wbuf->size);
    newconn->accepting  = ZC_FALSE;
    newconn->connected  = ZC_TRUE;
    
    return newconn;
}

zcAsynIO*
zc_asynio_new_tcp_client(const char *host, int port, int timeout, zcProtocol *p, 
        struct ev_loop *loop, int rbufsize, int wbufsize)
{
    zcSocket *sock = zc_socket_new_tcp(timeout);
    if (NULL == sock)
        return NULL;
    zc_socket_setblock(sock, ZC_FALSE);
    zcAsynIO *conn = zc_asynio_new(sock, p, loop, rbufsize, wbufsize);
    if (NULL == conn) {
        zc_socket_delete(sock);
        return NULL;
    }
    conn->accepting = ZC_FALSE;
    conn->connected = ZC_FALSE;

    int ret;
    ZCINFO("connect to %s:%d", host, port);
    ret = zc_socket_connect(conn->sock, (char*)host, port);
    if (ret < 0 && ret != -EINPROGRESS && ret != -EALREADY && ret != -EWOULDBLOCK) {
        ZCWARN("connect error! %d, %s\n", ret, strerror(-ret));
        /*if (conn->handle_connfailed) {
            conn->handle_connfailed(conn);
            zc_asynio_delete(conn);
        }*/
        conn->p.handle_error(conn, ret);
        zc_asynio_safedel(conn);
    }
    return conn;
}

zcAsynIO*
zc_asynio_new_ssl_client(const char *host, int port, int timeout, zcProtocol *p,
        struct ev_loop *loop, int rbufsize, int wbufsize)
{
    zcSocket *sock = zc_socket_new_tcp(timeout);
    if (NULL == sock)
        return NULL;
    zc_socket_setblock(sock, ZC_FALSE);
    zcAsynIO *conn = zc_asynio_new(sock, p, loop, rbufsize, wbufsize);
    if (NULL == conn) {
        zc_socket_delete(sock);
        return NULL;
    }
    conn->accepting = ZC_FALSE;
    conn->connected = ZC_FALSE;
    conn->ssl = true;

    int ret;
    ret = zc_socket_connect(conn->sock, (char*)host, port);
    if (ret < 0 && ret != -EINPROGRESS && ret != -EALREADY && ret != -EWOULDBLOCK) {
        ZCWARN("connect error! %d\n", ret);
        /*if (conn->handle_connfailed) {
            conn->handle_connfailed(conn);
            zc_asynio_delete(conn);
        }*/
        conn->p.handle_error(conn, ret);
        zc_asynio_safedel(conn);
    }
    return conn;
}

zcAsynIO*
zc_asynio_new_udp_client(const char *host, int port, int timeout, zcProtocol *p, 
        struct ev_loop *loop, int rbufsize, int wbufsize)
{
    zcSocket *sock = zc_socket_new_udp(timeout);
    if (NULL == sock)
        return NULL;
    //zc_socket_setblock(sock, ZC_FALSE);
    zc_socket_add_remote_addr(sock, host, port);
    zcAsynIO *conn = zc_asynio_new(sock, p, loop, rbufsize, wbufsize);
    if (NULL == conn) {
        ZCERROR("zc_asynio_new is NULL");
        zc_socket_delete(sock);
        return NULL;
    }
    return conn;
}


zcAsynIO*
zc_asynio_new_tcp_server(const char *host, int port, int timeout, zcProtocol *p,
        struct ev_loop *loop, int rbufsize, int wbufsize)
{
    zcSocket *sock = zc_socket_server_tcp((char*)host, port, 256);
    if (NULL == sock)
        return NULL;
    zc_socket_setblock(sock, ZC_FALSE);
    zcAsynIO *conn = zc_asynio_new(sock, p, loop, rbufsize, wbufsize);
    if (NULL == conn) {
        zc_socket_delete(sock);
        return NULL;
    }
    sock->timeout = timeout;
    
    conn->connected = ZC_TRUE;
    conn->accepting = ZC_TRUE;

    return conn;
}


zcAsynIO*
zc_asynio_new_udp_server(const char *host, int port, int timeout, zcProtocol *p,
        struct ev_loop *loop, int rbufsize, int wbufsize)
{
    zcSocket *sock = zc_socket_server_udp((char*)host, port, 256);
    if (NULL == sock)
        return NULL;
    //zc_socket_setblock(sock, ZC_FALSE);
    zcAsynIO *conn = zc_asynio_new(sock, p, loop, rbufsize, wbufsize);
    if (NULL == conn) {
        zc_socket_delete(sock);
        return NULL;
    }
    zc_socket_reuse(sock);
    sock->timeout = timeout;
    
    conn->connected = ZC_TRUE;
    conn->accepting = ZC_TRUE;

    return conn;
}

void
zc_asynio_set_protocol(zcAsynIO *conn, zcProtocol *p)
{
    memcpy(&conn->p, p, sizeof(zcProtocol));
}

void
zc_asynio_copy(zcAsynIO *conn, zcAsynIO *fromconn)
{
    zc_asynio_set_protocol(conn, &fromconn->p);
    //conn->calls = fromconn->calls;
}

int
zc_asynio_connect(zcAsynIO *conn, const char *host, int port)
{
    conn->accepting = ZC_FALSE;
    conn->connected = ZC_FALSE;

    int ret;
    ret = zc_socket_connect(conn->sock, (char*)host, port);
    if (ret < 0 && ret != -EINPROGRESS && ret != -EALREADY && ret != -EWOULDBLOCK) {
        ZCWARN("connect error! %d\n", ret);
        /*if (conn->handle_connfailed) {
            conn->handle_connfailed(conn);
        }*/
        conn->p.handle_error(conn, ret);
    }

    return ZC_OK; 
}

typedef int (*zcFuncAsynTimerCallback)(zcAsynIO*, void *data);

struct zc_asyn_callback_value_t
{
    ev_timer    timer;
    zcFuncAsynTimerCallback callback;
    zcAsynIO  *conn;
    void        *data;
};

static void 
_call_later_ev_timeout(struct ev_loop *loop, ev_timer *t, int events)
{
    struct zc_asyn_callback_value_t *v = t->data;
    
    double repeat_time = (double)t->repeat;
    int ret = v->callback(v->conn, v->data);
    //ZCINFO("repeat:%d, ret:%lf", (double)t->repeat, ret);
    if (ret == ZC_STOP || fabs(repeat_time) <= EPSINON) {
        ZCINFO("stop timer");
        ev_timer_stop(loop, t);
        zc_free(v);
    }
}

int
zc_asynio_call_later(zcAsynIO *conn, int after, int repeat, 
        int (*callback)(zcAsynIO*, void *data), void *data)
{
    struct zc_asyn_callback_value_t *v = zc_calloct(struct zc_asyn_callback_value_t);
    //ev_timer timer; 
    double after2  = after/1000.0;
    double repeat2 = repeat/1000.0;
    ev_timer_init(&v->timer, _call_later_ev_timeout, after2, repeat2);

    v->callback = callback;
    v->conn = conn;
    v->data = data;
    v->timer.data = v;

    ev_timer_start(conn->loop, &v->timer);
    
    return ZC_OK;
}

int
zc_asynio_read_bytes(zcAsynIO *conn, int len, zcFuncReadx callback)
{
    conn->_read_callback = callback;
    conn->_read_bytes = len;
    conn->_read_until = NULL;

    return ZC_OK;
}

int
zc_asynio_read_until(zcAsynIO *conn, char *s, zcFuncReadx callback)
{
    conn->_read_callback = callback;
    conn->_read_until = s; // note: not need free in asynio object
    conn->_read_bytes = 0;

    return ZC_OK;
}

void
zc_asynio_delete(void* conn)
{
    zcAsynIO *aconn = (zcAsynIO*)conn;
    //ZCINFO("read buffer delete");
    zcBuffer *tmp;
    if (aconn->rbuf_free) {
        zcBuffer *buf = aconn->rbuf;
        while (buf) {
            tmp = buf->next;
            ZCDEBUG("free rbuf:%p", buf);
            zc_buffer_delete(buf);
            buf = tmp;
        }
    }
    //ZCINFO("write buffer delete");
    if (aconn->wbuf_free) {
        zcBuffer *buf = aconn->wbuf;
        while (buf) {
            tmp = buf->next;
            zc_buffer_delete(buf);
            buf = tmp;
        }
    }

    //ZCINFO("stop io in loop");
    ev_io_stop(aconn->loop, &aconn->r);
    ev_io_stop(aconn->loop, &aconn->w);

    ev_timer_stop(aconn->loop, &aconn->rtimer);
    ev_timer_stop(aconn->loop, &aconn->wtimer);

    //if (aconn->read_timeout > 0) {
    if (aconn->timer.pending > 0 || aconn->timer.active > 0) {
        ev_timer_stop(aconn->loop, &aconn->timer);
    }
    if (aconn->sock) {
        zc_socket_delete(aconn->sock);
    }
    /*if (aconn->calls) {
        zc_callchain_safedel(aconn->calls);    
    }*/

    zc_free(conn);
}

static void
_call_later_delete_conn(struct ev_loop *loop, ev_timer *t, int events)
{
    zcAsynIO *conn = (zcAsynIO*)t->data;
    zc_asynio_safedel(conn);
}

// delete zcAsynIO in next event loop
void
zc_asynio_delete_delay(void* c)
{
    ZCDEBUG("delete delay conn:%p", c);
    zcAsynIO *conn = (zcAsynIO*)c;
    // have timer, must stop
    if (conn->timer.pending > 0 || conn->timer.active > 0) {
        ev_timer_stop(conn->loop, &conn->timer);
    }
    float tm = 0;
    ev_timer_init(&conn->timer, _call_later_delete_conn, tm, tm);
    conn->timer.data = conn;
    ev_timer_start(conn->loop, &conn->timer);

    //ZCINFO("stop io in loop");
    ev_io_stop(conn->loop, &conn->r);
    ev_io_stop(conn->loop, &conn->w);

    ev_timer_stop(conn->loop, &conn->rtimer);
    ev_timer_stop(conn->loop, &conn->wtimer);
}



#endif
