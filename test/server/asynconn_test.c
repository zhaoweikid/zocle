#include <zocle/zocle.h>
#include <assert.h>
#include <ev.h>

int my_handle_ready(zcAsynConn *conn, char *data, int datalen)
{
    char buf[1024] = {0};
    strncpy(buf, data, datalen);
    ZCNOTE("%d ok, found line len:%d, %s", conn->sock->fd, datalen, buf);

    zc_buffer_append(conn->wbuf, "ok\r\n", 4); 
    return ZC_OK;
}


int
my_handle_accept(zcAsynConn *conn)
{
    //ZCINFO("try accept");
    zcSocket *newsock = zc_socket_accept(conn->sock);
    if (NULL == newsock)
        return ZC_OK;
    //zc_check(newsock);
    ZCINFO("ok, accept! new:%d", newsock->fd);
    newsock->timeout    = conn->sock->timeout;
    zcAsynConn *newconn = zc_asynconn_new(newsock, conn->loop, conn->rbuf->size, conn->wbuf->size);
    newconn->accepting  = ZC_FALSE;
    newconn->connected  = ZC_TRUE;
    //zc_asynconn_copy(newconn, conn);
    newconn->handle_ready  = my_handle_ready;

    zc_socket_linger(newconn->sock, 1, 0);

    return ZC_OK;
}

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);
    
    struct ev_loop *loop = ev_default_loop (0);

    zcAsynConn *conn = zc_asynconn_new_tcp_server("127.0.0.1", 10000, 5000, loop, 1024, 1024);
    if (NULL == conn) {
        ZCERROR("server create error");
        return 0;
    }
    //conn->handle_ready  = my_handle_ready;
    conn->handle_accept = my_handle_accept;

    zc_check(conn);
    zc_check(conn->sock);
    zc_check(conn->rbuf);
    zc_check(conn->wbuf);
   
    ZCINFO("started");
    ev_run (loop, 0);
    ZCINFO("stopped");
    
    zc_asynconn_delete(conn);

    return 0;
}
