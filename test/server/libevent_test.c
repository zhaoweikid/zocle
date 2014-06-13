#include <zocle/zocle.h>
#include <event2/event.h>

typedef struct _conn
{
    char     wbuf[128];
    char     rbuf[128];
    zcSocket *sock;
    struct event r;
    struct event w;
}Conn;

void write_cb(int fd, short event, void *arg)
{
    Conn *conn = (Conn*)arg;

    int ret = zc_socket_send(conn->sock, conn->wbuf, strlen(conn->wbuf));
    if (ret < 0) {
        ZCWARN("send error! %s\n", strerror(-1*ret));
        event_del(&conn->r);
        event_del(&conn->w);
        
        zc_socket_delete(conn->sock);
        zc_free(conn);
        return;
    }
    event_del(&conn->w);
}


void read_cb(int fd, short event, void *arg)
{
    Conn *conn = (Conn*)arg;
    
    int ret = zc_socket_recv(conn->sock, conn->rbuf, sizeof(conn->rbuf));
    if (ret <= 0) {
        event_del(&conn->r);
        event_del(&conn->w);

        zc_socket_delete(conn->sock);
        zc_free(conn);
        return;  
    }
    conn->rbuf[ret] = 0;

    if (strstr(conn->rbuf, "\r\n") != NULL) {
        strcpy(conn->wbuf, "ok\r\n");

        event_set(&conn->w, conn->sock->fd, EV_WRITE, write_cb, (void *)conn);
        event_add(&conn->w, 0);
    }
}


void
read_accept_cb(int fd, short event, void *arg)
{
    zcSocket *sock = (zcSocket*)arg;

    zcSocket *newsock = zc_socket_accept(sock);
    if (newsock == NULL) {
        return;
    }
    Conn *conn = (Conn*)zc_malloc(sizeof(Conn));
    memset(conn, 0, sizeof(Conn));
    conn->sock = newsock;
    
    event_set(&conn->r, conn->sock->fd, EV_READ|EV_PERSIST, read_cb, (void *)conn);
    event_add(&conn->r, 0);
}

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);

    zcSocket    *sock = zc_socket_server_tcp("127.0.0.1", 10000, 256);
    if (NULL == sock) {
        ZCERROR("socket create error!");
        return -1;
    }

    event_init();

    struct event myev;
    event_set(&myev, sock->fd, EV_READ|EV_PERSIST, read_accept_cb, (void *)sock);
    event_add(&myev, 0);

    event_dispatch();

    return 0;
}
