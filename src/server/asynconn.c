#ifdef ZOCLE_WITH_LIBEV

#include <zocle/server/asynconn.h>
#include <zocle/log/logfile.h>
#include <zocle/base/defines.h>
#include <zocle/mem/alloc.h>
#include <zocle/str/buffer.h>
#include <errno.h>
#include <unistd.h>

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
zc_callchain_call(zcCallChain *cc, zcAsynConn *conn, int ret)
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



zcSocket*
zc_asynconn_handle_accept_one(zcAsynConn *conn)
{
    zcSocket *newsock = zc_socket_accept(conn->sock);
    if (NULL == newsock) {
        return NULL;
    }
    newsock->timeout = conn->sock->timeout;
    zcAsynConn *newconn = zc_asynconn_new(newsock, conn->p, conn->loop, conn->rbuf->size, conn->wbuf->size);
    newconn->accepting = ZC_FALSE;
    newconn->connected = ZC_TRUE;
    zc_asynconn_copy(newconn, conn);

    return newsock;
}


int
zc_asynconn_handle_accept(zcAsynConn *conn)
{
    int i;
    for (i=0; i<ZC_ONE_ACCEPT_MAX; i++) {
        zcSocket *newsock = zc_asynconn_handle_accept_one(conn);
        if (NULL == newsock)
            break;
    }
    return ZC_OK;
}
int
zc_asynconn_handle_connected(zcAsynConn *conn)
{
    ZCNOTE("connected");
    if (conn->ssl) {
        // async ssl?
        // zc_socket_client_ssl(conn->sock, NULL, NULL);
    }
    return ZC_OK;
}
/*int
zc_asynconn_handle_connlost(zcAsynConn *conn)
{
    ZCNOTE("connection lost");
    return ZC_OK;
}
int
zc_asynconn_handle_connfailed(zcAsynConn *conn)
{
    ZCNOTE("connection failed");
    return ZC_OK;
}*/

int
zc_asynconn_handle_error(zcAsynConn *conn, int err)
{
    ZCNOTE("connection error! %d", err);
    return ZC_OK;
}


int
zc_asynconn_handle_close(zcAsynConn *conn)
{
    ZCNOTE("connection %s:%d close (%p)", conn->sock->remote.ip, conn->sock->remote.port, conn);
    return ZC_OK;
}
int
zc_asynconn_handle_read(zcAsynConn *conn, zcBuffer *buf)
{
    const char *data = buf->data+buf->pos;
    ZCINFO("check data:%s, len:%d\n", data, zc_buffer_used(buf));
    char *pos = strstr(data, "\r\n");
    if (pos == NULL)
        return 0;
    if (zc_buffer_used(buf) >= 8192)
        return ZC_ERR;

    zc_buffer_append(conn->wbuf, "ok\r\n", 4);

    return pos-data+2;
}

/*int
zc_asynconn_handle_ready(zcAsynConn *conn, char *data, int datalen)
{
    char buf[1024] = {0};
    strncpy(buf, data, datalen);
    //ZCNOTE("ok, found line len:%d, %s", datalen, buf);

    zc_buffer_append(conn->wbuf, "ok\r\n", 4);
    return ZC_OK;
}*/

int
zc_asynconn_handle_wrote(zcAsynConn *conn)
{
    //ZCNOTE("data wrote");
    return ZC_OK;
}
int
zc_asynconn_handle_read_timeout(zcAsynConn *conn)
{
    ZCNOTE("read timeout, close conn");
    conn->p->handle_close(conn);
    //zc_asynconn_delete(conn);
    zc_asynconn_safedel(conn);
    return ZC_OK;
}
int
zc_asynconn_handle_write_timeout(zcAsynConn *conn)
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
    p->handle_accept     = zc_asynconn_handle_accept;;
    p->handle_connected  = zc_asynconn_handle_connected;
    p->handle_error      = zc_asynconn_handle_error;
    p->handle_close      = zc_asynconn_handle_close;
    p->handle_read       = zc_asynconn_handle_read;
    p->handle_wrote      = zc_asynconn_handle_wrote;
    p->handle_read_timeout  = zc_asynconn_handle_read_timeout;
    p->handle_write_timeout = zc_asynconn_handle_write_timeout;
    return ZC_OK;
}






#define ZC_ONE_READ_MAX 8192

static void 
zc_asynconn_ev_read(struct ev_loop *loop, ev_io *r, int events)
{
    zcAsynConn *conn = (zcAsynConn*)r->data; 
    ZCINFO("ok, read event, conn:%p\n", conn);
    int ret = 0;
    ev_timer_stop(loop, &conn->timer);
    // udp
    if (conn->sock->type == SOCK_DGRAM) {
        zcBuffer *rbuf = conn->rbuf;
        ret = zc_socket_recvfrom_self(conn->sock, zc_buffer_data(rbuf), zc_buffer_idle(rbuf), 0);
        if (ret <= 0) {
            ZCNOTE("recvfrom error:%d, close conn", ret);
            conn->p->handle_close(conn);
            zc_asynconn_safedel(conn);
            return;
        }
        ZCINFO("udp read:%d", ret);
        //conn->handle_ready(conn, rbuf->data+rbuf->pos, ret);
        conn->p->handle_read(conn, rbuf);
        zc_buffer_reset(rbuf);
        //ZCINFO("rbuffer reset");
        if (zc_buffer_used(conn->wbuf) > 0) {
#ifdef ASYNC_ONE_WATCHER
            zc_asynconn_switch_write(conn);
#else
            zc_asynconn_start_write(conn);
#endif
        }
        return;
    }

    // tcp
    if (conn->accepting) {
        conn->p->handle_accept(conn);
        return;
    }
    if (conn->connected == ZC_FALSE) {
        //ZCINFO("try do connect\n");
        conn->p->handle_connected(conn);
        conn->connected = ZC_TRUE;
    }
    //ZCINFO("try read data\n");
    zcBuffer *rbuf = conn->rbuf;
    int rsize = 0;
    while (zc_buffer_idle(rbuf) > 0) {
        ret = zc_socket_recv(conn->sock, rbuf->data+rbuf->end, 
                zc_buffer_idle(rbuf)); 
        if (ret == 0 || (ret < 0 && ret != -EWOULDBLOCK && ret != -EAGAIN)) {
            ZCNOTE("recv error:%d, close conn", ret);
            //conn->handle_connlost(conn);
            if (ret == 0) {
                conn->close = true;
                if (zc_buffer_used(rbuf) > 0) {
                    //conn->handle_ready(conn, rbuf->data+rbuf->pos, zc_buffer_used(rbuf));
                    conn->p->handle_read(conn, rbuf);
                }
            }
            conn->p->handle_close(conn);
            //zc_socket_close(conn->sock);
            zc_asynconn_safedel(conn);
            return;
        }
        if (ret < 0) { // EWOULDBLOCK, EAGAIN
            break;
        }
        rbuf->end += ret;
        rsize += ret;
        if (rsize >= ZC_ONE_READ_MAX) // read max 8192, then break
            break;
    }
    while (zc_buffer_used(rbuf) > 0) {
        //ret = conn->handle_read_check(conn, rbuf->data+rbuf->pos, rbuf->len-rbuf->pos);
        ret = conn->p->handle_read(conn, rbuf);
        //ZCINFO("read check ret:%d, buffer len:%d\n", ret, zc_buffer_used(rbuf));
        if (ret < 0) {
            ZCWARN("handle_read failed %d, break", ret);
            conn->p->handle_close(conn);
            zc_asynconn_safedel(conn);
            //break;
            return;
        }
        if (ret == 0) // read on next
            break;
        //if (conn->handle_ready)
        //    conn->handle_ready(conn, rbuf->data+rbuf->pos, ret);
        rbuf->pos += ret;
    }
    zc_buffer_compact(conn->rbuf);

    if (zc_buffer_used(conn->wbuf) > 0) {
#ifdef ASYNC_ONE_WATCHER
        zc_asynconn_switch_write(conn);
#else
        zc_asynconn_start_write(conn);
#endif
    }
    //ZCINFO("read event end");
    return;
}

static void 
zc_asynconn_ev_write(struct ev_loop *loop, ev_io *w, int events)
{
    //ZCINFO("ok, write event\n");
    zcAsynConn *conn = (zcAsynConn*)w->data; 
    ev_timer_stop(loop, &conn->timer);
    int ret;
    zcBuffer *wbuf = conn->wbuf;
    // udp
    if (conn->sock->type == SOCK_DGRAM) {
        ret = zc_socket_sendto_self(conn->sock, zc_buffer_data(wbuf), zc_buffer_used(wbuf), 0);
        if (ret != zc_buffer_used(wbuf)) {
            ZCWARN("sendto error:%d, wbuf:%d, close conn", ret, zc_buffer_used(wbuf));
            conn->p->handle_close(conn);
            zc_asynconn_safedel(conn);
            return;
        }
        ZCINFO("udp write:%d", ret);
        conn->p->handle_wrote(conn);
        zc_buffer_reset(wbuf);

        if (zc_buffer_used(conn->wbuf) == 0) {
#ifdef ASYNC_ONE_WATCHER
            zc_asynconn_switch_read(conn);
#else
            ev_io_stop(loop, &conn->w);
#endif
        }
        return;
    }

    // tcp
    ret = zc_socket_peek(conn->sock);
    if (ret <= 0 && ret != -EAGAIN) {
        if (zc_socket_conn_lost(-1*ret)) {
            //conn->handle_connlost(conn);
            conn->p->handle_error(conn, ret);
        }
        ZCWARN("peek error:%d\n", ret);
        conn->p->handle_close(conn); 
        //zc_socket_delete(conn->sock);
        zc_asynconn_safedel(conn);
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
            conn->p->handle_close(conn);
            //zc_socket_delete(conn->sock);
            zc_asynconn_safedel(conn);
            return;
        }
        wbuf->pos += ret;
    }
    ZCINFO("after send, wbuf:%d", zc_buffer_used(conn->wbuf));
    if (zc_buffer_used(conn->wbuf) == 0) {
        if (conn->wbuf->next == NULL) {
            zc_buffer_reset(conn->wbuf);
            conn->p->handle_wrote(conn);
#ifdef  ASYNC_ONE_WATCHER 
            zc_asynconn_switch_read(conn);
#else
            ev_io_stop(loop, &conn->w);
#endif
        }else{
            zcBuffer *mybuf = conn->wbuf;
            conn->wbuf = conn->wbuf->next;
            zc_buffer_delete(mybuf);
        }
    }
    //ZCINFO("write event end");
    return;
}

#ifdef ASYNC_ONE_WATCHER
static void 
zc_asynconn_ev_timeout(struct ev_loop *loop, ev_timer *r, int events)
{
    zcAsynConn *conn = (zcAsynConn*)r->data; 
    if (conn->watcher.events & EV_READ) {
        conn->p->handle_read_timeout(conn);
    }else if (conn->watcher.events & EV_WRITE) {
        conn->p->handle_write_timeout(conn);
    }
}

#else
static void 
zc_asynconn_read_ev_timeout(struct ev_loop *loop, ev_timer *r, int events)
{
    zcAsynConn *conn = (zcAsynConn*)r->data; 
    conn->p->handle_read_timeout(conn);
}

static void 
zc_asynconn_write_ev_timeout(struct ev_loop *loop, ev_timer *r, int events)
{
    zcAsynConn *conn = (zcAsynConn*)r->data; 
    conn->p->handle_write_timeout(conn);
}

#endif

zcAsynConn* 
zc_asynconn_new(zcSocket *sock, zcProtocol *p, struct ev_loop *loop, int rbufsize, int wbufsize)
{
    zcAsynConn *conn = NULL;
    conn = zc_calloct(zcAsynConn);
    zc_socket_setblock(sock, ZC_FALSE);

    conn->rbuf = zc_buffer_new(rbufsize);
    conn->wbuf = zc_buffer_new(wbufsize);
    /*conn->connected = ZC_FALSE;
    conn->accepting = ZC_FALSE;
    conn->close= ZC_FALSE;
    conn->ssl  = ZC_FALSE;*/
    conn->sock = sock;
    conn->loop = loop;

    conn->p = p;
    /*conn->handle_accept     = zc_asynconn_handle_accept;;
    conn->handle_connected  = zc_asynconn_handle_connected;
    conn->handle_error      = zc_asynconn_handle_error;
    conn->handle_close      = zc_asynconn_handle_close;
    conn->handle_read       = zc_asynconn_handle_read;
    //conn->handle_ready      = zc_asynconn_handle_ready;
    conn->handle_wrote      = zc_asynconn_handle_wrote;
    conn->handle_read_timeout  = zc_asynconn_handle_read_timeout;
    conn->handle_write_timeout = zc_asynconn_handle_write_timeout;
    */

#ifdef ASYNC_ONE_WATCHER 
    ev_io_init(&conn->watcher, zc_asynconn_ev_read, conn->sock->fd, EV_READ);
    conn->watcher.data = conn;
    ev_io_start(conn->loop, &conn->watcher);
#else
    ev_io_init(&conn->r, zc_asynconn_ev_read, conn->sock->fd, EV_READ);
    conn->r.data = conn;
    ev_io_start(conn->loop, &conn->r);
#endif
    int timeout = sock->timeout;
    conn->read_timeout = conn->write_timeout = conn->conn_timeout = timeout;
    //ZCINFO("timeout:%d\n", timeout);
    if (timeout > 0) {
        //ev_timer_init(&conn->timer, zc_asynconn_ev_timeout, timeout, 1);
        float tm = timeout/1000.0;
#ifdef ASYNC_ONE_WATCHER
        ev_timer_init(&conn->timer, zc_asynconn_ev_timeout, tm, tm);
        conn->timer.data = conn;
        ev_timer_start(conn->loop, &conn->timer);
#else
        ev_timer_init(&conn->timer, zc_asynconn_read_ev_timeout, tm, tm);
        conn->timer.data = conn;
        ev_timer_start(conn->loop, &conn->timer);
#endif
    }

    return conn;
}

zcAsynConn*
zc_asynconn_new_tcp_client(const char *host, int port, int timeout, zcProtocol *p, 
        struct ev_loop *loop, int rbufsize, int wbufsize)
{
    zcSocket *sock = zc_socket_new_tcp(timeout);
    if (NULL == sock)
        return NULL;
    zc_socket_setblock(sock, ZC_FALSE);
    zcAsynConn *conn = zc_asynconn_new(sock, p, loop, rbufsize, wbufsize);
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
            zc_asynconn_delete(conn);
        }*/
        conn->p->handle_error(conn, ret);
        zc_asynconn_safedel(conn);
    }
    return conn;
}

zcAsynConn*
zc_asynconn_new_ssl_client(const char *host, int port, int timeout, zcProtocol *p,
        struct ev_loop *loop, int rbufsize, int wbufsize)
{
    zcSocket *sock = zc_socket_new_tcp(timeout);
    if (NULL == sock)
        return NULL;
    zc_socket_setblock(sock, ZC_FALSE);
    zcAsynConn *conn = zc_asynconn_new(sock, p, loop, rbufsize, wbufsize);
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
            zc_asynconn_delete(conn);
        }*/
        conn->p->handle_error(conn, ret);
        zc_asynconn_safedel(conn);
    }
    return conn;
}

zcAsynConn*
zc_asynconn_new_udp_client(const char *host, int port, int timeout, zcProtocol *p, 
        struct ev_loop *loop, int rbufsize, int wbufsize)
{
    zcSocket *sock = zc_socket_new_udp(timeout);
    if (NULL == sock)
        return NULL;
    //zc_socket_setblock(sock, ZC_FALSE);
    zc_socket_add_remote_addr(sock, host, port);
    zcAsynConn *conn = zc_asynconn_new(sock, p, loop, rbufsize, wbufsize);
    if (NULL == conn) {
        ZCERROR("zc_asynconn_new is NULL");
        zc_socket_delete(sock);
        return NULL;
    }
    return conn;
}


zcAsynConn*
zc_asynconn_new_tcp_server(const char *host, int port, int timeout, zcProtocol *p,
        struct ev_loop *loop, int rbufsize, int wbufsize)
{
    zcSocket *sock = zc_socket_server_tcp((char*)host, port, 256);
    if (NULL == sock)
        return NULL;
    zc_socket_setblock(sock, ZC_FALSE);
    zcAsynConn *conn = zc_asynconn_new(sock, p, loop, rbufsize, wbufsize);
    if (NULL == conn) {
        zc_socket_delete(sock);
        return NULL;
    }
    sock->timeout = timeout;
    
    conn->connected = ZC_TRUE;
    conn->accepting = ZC_TRUE;

    return conn;
}

zcAsynConn*
zc_asynconn_new_udp_server(const char *host, int port, int timeout, zcProtocol *p,
        struct ev_loop *loop, int rbufsize, int wbufsize)
{
    zcSocket *sock = zc_socket_server_udp((char*)host, port, 256);
    if (NULL == sock)
        return NULL;
    //zc_socket_setblock(sock, ZC_FALSE);
    zcAsynConn *conn = zc_asynconn_new(sock, p, loop, rbufsize, wbufsize);
    if (NULL == conn) {
        zc_socket_delete(sock);
        return NULL;
    }
    sock->timeout = timeout;
    
    conn->connected = ZC_TRUE;
    conn->accepting = ZC_TRUE;

    return conn;
}

void
zc_asynconn_copy(zcAsynConn *conn, zcAsynConn *fromconn)
{
    /*conn->handle_accept     = fromconn->handle_accept;
    conn->handle_connected  = fromconn->handle_connected;
    conn->handle_error      = fromconn->handle_error;
    conn->handle_close      = fromconn->handle_close;
    conn->handle_read       = fromconn->handle_read;
    //conn->handle_ready      = fromconn->handle_ready;
    conn->handle_wrote      = fromconn->handle_wrote;
    conn->handle_read_timeout  = fromconn->handle_read_timeout;
    conn->handle_write_timeout = fromconn->handle_write_timeout;
    */
    conn->p = fromconn->p;
    conn->calls = fromconn->calls;
}

int
zc_asynconn_connect(zcAsynConn *conn, const char *host, int port)
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
        conn->p->handle_error(conn, ret);
    }

    return ZC_OK; 
}

typedef int (*zcFuncAsynTimerCallback)(zcAsynConn*, void *data);

struct zc_asyn_callback_value_t
{
    ev_timer    timer;
    zcFuncAsynTimerCallback callback;
    zcAsynConn  *conn;
    void        *data;
};

static void 
_call_later_ev_timeout(struct ev_loop *loop, ev_timer *t, int events)
{
    struct zc_asyn_callback_value_t *v = t->data;

    int ret = v->callback(v->conn, v->data);
    if (t->repeat) {
        if (ret < 0) {
            ZCINFO("callback return err, stop repeat timer");
            ev_timer_stop(loop, t);
            zc_free(v);
        }
    }else{
        ZCINFO("stop timer");
        ev_timer_stop(loop, t);
        zc_free(v);
    }
}

int
zc_asynconn_call_later(zcAsynConn *conn, int after, int repeat, 
        int (*callback)(zcAsynConn*, void *data), void *data)
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


void
zc_asynconn_delete(void* conn)
{
    zcAsynConn *aconn = (zcAsynConn*)conn;
    //ZCINFO("read buffer delete");
    zcBuffer *tmp;
    zcBuffer *buf = aconn->rbuf;
    while (buf) {
        tmp = buf->next;
        zc_buffer_delete(buf);
        buf = tmp;
    }
    //ZCINFO("write buffer delete");
    buf = aconn->wbuf;
    while (buf) {
        tmp = buf->next;
        zc_buffer_delete(buf);
        buf = tmp;
    }

    //ZCINFO("stop io in loop");
#ifdef ASYNC_ONE_WATCHER
    ev_io_stop(aconn->loop, &aconn->watcher);
#else
    ev_io_stop(aconn->loop, &aconn->r);
    ev_io_stop(aconn->loop, &aconn->w);
#endif

    //if (aconn->read_timeout > 0) {
    if (aconn->timer.pending > 0 || aconn->timer.active > 0) {
        ev_timer_stop(aconn->loop, &aconn->timer);
    }
    if (aconn->sock) {
        zc_socket_close(aconn->sock);
    }
    if (aconn->calls) {
        zc_callchain_safedel(aconn->calls);    
    }

    zc_free(conn);
}

static void
_call_later_delete_conn(struct ev_loop *loop, ev_timer *t, int events)
{
    zcAsynConn *conn = (zcAsynConn*)t->data;
    zc_asynconn_safedel(conn);
}

void
zc_asynconn_delete_delay(void* c)
{
    zcAsynConn *conn = (zcAsynConn*)c;
    //if (conn->read_timeout > 0) {
    if (conn->timer.pending > 0 || conn->timer.active > 0) {
        ev_timer_stop(conn->loop, &conn->timer);
    }
    float tm = 0;
    ev_timer_init(&conn->timer, _call_later_delete_conn, tm, tm);
    conn->timer.data = conn;
    ev_timer_start(conn->loop, &conn->timer);
}

#ifdef ASYNC_ONE_WATCHER
void
zc_asynconn_switch_read(zcAsynConn *conn)
{
    //ZCINFO("switch to read");
    ev_io_stop(conn->loop, &conn->watcher); 

    ev_io_init(&conn->watcher, zc_asynconn_ev_read, conn->sock->fd, EV_READ);
    conn->watcher.data = conn;
    ev_io_start(conn->loop, &conn->watcher);

    ev_timer_again(conn->loop, &conn->timer);
}

void
zc_asynconn_switch_write(zcAsynConn *conn)
{
    //ZCINFO("switch to write");
    ev_io_stop(conn->loop, &conn->watcher);

    ev_io_init(&conn->watcher, zc_asynconn_ev_write, conn->sock->fd, EV_WRITE);
    conn->watcher.data = conn;
    ev_io_start(conn->loop, &conn->watcher);

    ev_timer_again(conn->loop, &conn->timer);
}

#else

void
zc_asynconn_start_write(zcAsynConn *conn)
{
    ev_io_init(&conn->w, zc_asynconn_ev_write, conn->sock->fd, EV_WRITE);
    conn->w.data = conn;
    ev_io_start(conn->loop, &conn->w);
}

#endif



#endif
