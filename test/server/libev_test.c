#include <zocle/zocle.h>
#include <ev.h>

typedef struct _conn
{
    char     wbuf[128];
    char     rbuf[128];
    zcSocket *sock;
    ev_io    r;
    ev_io    w;
}Conn;

static void write_cb (struct ev_loop *loop, ev_io *w, int revents)
{
    Conn *conn = w->data;

    int ret = zc_socket_send(conn->sock, conn->wbuf, strlen(conn->wbuf));
    if (ret < 0) {
        ev_io_stop(loop, &conn->r);
        ev_io_stop(loop, &conn->w);
        zc_socket_delete(conn->sock);
        zc_free(conn);
        return;
    }
    ev_io_stop(loop, &conn->w);
}


static void read_cb (struct ev_loop *loop, ev_io *w, int revents)
{
    Conn *conn = w->data;
    
    int ret = zc_socket_recv(conn->sock, conn->rbuf, sizeof(conn->rbuf));
    if (ret <= 0) {
        ev_io_stop(loop, &conn->r);
        ev_io_stop(loop, &conn->w);
        zc_socket_delete(conn->sock);
        zc_free(conn);
        return;  
    }
    conn->rbuf[ret] = 0;

    if (strstr(conn->rbuf, "\r\n") != NULL) {
        strcpy(conn->wbuf, "ok\r\n");
        ev_io_init (&conn->w, write_cb, conn->sock->fd, EV_WRITE);
        conn->w.data = conn;
        ev_io_start (loop, &conn->w);
    }
}



static void read_accept_cb (struct ev_loop *loop, ev_io *w, int revents)
{
    zcSocket *sock = w->data;

    zcSocket *newsock = zc_socket_accept(sock);
    if (newsock == NULL) {
        return;
    }
    Conn *conn = (Conn*)zc_malloc(sizeof(Conn));
    memset(conn, 0, sizeof(Conn));
    conn->sock = newsock;

    ev_io_init (&conn->r, read_cb, newsock->fd, EV_READ);
    conn->r.data = conn;
    ev_io_start (loop, &conn->r);
}

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);

    zcSocket *sock = zc_socket_server_tcp("0.0.0.0", 10000, 256);
    if (NULL == sock) {
        ZCERROR("socket create error!");
        return -1;
    }

    struct ev_loop *loop = ev_default_loop (0);

    ev_io watcher;
    ev_io_init (&watcher, read_accept_cb, sock->fd, EV_READ);
    watcher.data = sock;
    ev_io_start (loop, &watcher);

    ev_run (loop, 0);

    return 0;
}
