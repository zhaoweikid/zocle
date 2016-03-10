#include <zocle/zocle.h>
#include <assert.h>
#include <ev.h>

int my_handle_read_line(zcAsynIO *conn, const char *data, int len)
{
    char input[1024] = {0};
    strncpy(input, data, len-2);
    ZCINFO("input:%s", input);
    
    char wbuf[1024] = {0};
    int wlen = snprintf(wbuf, sizeof(wbuf), "ok! %s ~~~\r\n", input);
    ZCINFO("wlen:%d, %s", wlen, wbuf);
    zc_buffer_append(conn->wbuf, wbuf, wlen); 

    //zc_asynio_read_until(conn, "\r\n", my_handle_read_line);

    return len;
}

int my_call_later(zcAsynIO *conn, void *data)
{
    char *s = (char*)data;

    ZCDEBUG("later %s", s);

    return ZC_OK;
    //return ZC_STOP;
}


int
my_handle_connected(zcAsynIO *conn) 
{
    ZCINFO("connected!");
    zc_socket_linger(conn->sock, 1, 0);
    zc_asynio_read_until(conn, "\r\n", my_handle_read_line);

    return ZC_OK;
}

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);
    zcProtocol p;

    zc_protocol_init(&p);
    p.handle_connected = my_handle_connected;
    
    struct ev_loop *loop = ev_default_loop (0);

    zcAsynIO *conn = zc_asynio_new_tcp_server("127.0.0.1", 10000, 1000, &p, loop, 1024, 1024);
    if (NULL == conn) {
        ZCERROR("server create error");
        return 0;
    }

    zc_asynio_call_later(conn, 0, 0, my_call_later, "good");


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
