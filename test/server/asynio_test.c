#include <zocle/zocle.h>
#include <assert.h>
#include <ev.h>

int my_handle_read(zcAsynIO *conn)
{
    zcBuffer *buf = conn->rbuf;
    const char *data = zc_buffer_data(buf);
    int len = zc_buffer_used(buf);
    ZCINFO("used:%d", zc_buffer_used(buf));
    const char *pos = zc_strnstr(data, "\n", zc_buffer_used(buf));
    if (pos == NULL) {
        ZCINFO("not found \\n, again");
        return ZC_AGAIN;
    }
    char input[1024] = {0};
    strncpy(input, data, pos-data-1);
    ZCINFO("input:%s", input);
    
    char wbuf[1024] = {0};
    int wlen = snprintf(wbuf, sizeof(wbuf), "ok! %s ~~~\r\n", input);
    ZCINFO("wlen:%d, %s", wlen, wbuf);
    zc_buffer_append(conn->wbuf, wbuf, wlen); 

    return len;
}


/*int
my_handle_accept(zcAsynIO *conn)
{
    //ZCINFO("try accept");
    zcSocket *newsock = zc_socket_accept(conn->sock);
    if (NULL == newsock)
        return ZC_OK;
    //zc_check(newsock);
    ZCINFO("ok, accept! new:%d", newsock->fd);
    zcAsynIO *newconn = zc_asynio_new_accepted(newsock, conn);
    zc_socket_linger(newconn->sock, 1, 0);

    return ZC_OK;
}*/

int
my_handle_connected(zcAsynIO *conn) 
{
    zc_socket_linger(conn->sock, 1, 0);

    return ZC_OK;
}

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);
    zcProtocol p;

    zc_protocol_init(&p);
    //p.handle_accept = my_handle_accept;
    p.handle_connected = my_handle_connected;
    p.handle_read = my_handle_read;
    
    struct ev_loop *loop = ev_default_loop (0);

    zcAsynIO *conn = zc_asynio_new_tcp_server("127.0.0.1", 10000, 5000, &p, loop, 1024, 1024);
    if (NULL == conn) {
        ZCERROR("server create error");
        return 0;
    }

    zc_check(conn);
    zc_check(conn->sock);
    zc_check(conn->rbuf);
    zc_check(conn->wbuf);
   
    ZCINFO("started");
    ev_run (loop, 0);
    ZCINFO("stopped");
    
    zc_asynio_delete(conn);

    return 0;
}
