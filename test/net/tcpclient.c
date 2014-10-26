#include <zocle/zocle.h>

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);

    zcSocket    *sock;

    zc_socket_startup();

    sock = zc_socket_new_tcp(10);
    if (NULL == sock) {
        ZCERROR("tcp client create error!\n");
        return -1;
    }
    
    int ret = zc_socket_connect(sock, "127.0.0.1", 10000);
    if (ret != ZC_OK) {
        ZCINFO("connect 127.0.0.1:10000 error! %d\n", ret); 
        return -1;
    }

    ZCINFO("socket remote:%s:%d, local:%s:%d\n", 
            sock->remote.ip, sock->remote.port,
            sock->local.ip, sock->local.port);

    int i;
    for (i=0; i<10; i++) {
        char sendbuf[1024];
        char buf[1024];
        
        snprintf(sendbuf, sizeof(sendbuf), "good %d\r\n", i);
        ret = zc_socket_send(sock, sendbuf, strlen(sendbuf));
        if (ret != strlen(sendbuf)) {
            ZCERROR("send error! %d\n", ret);
            break;
        }
        ret = zc_socket_recv(sock, buf, 1024);
        if (ret > 0) {
            buf[ret] = 0;
        }
        if (ret == 0) {
            ZCINFO("read 0, reconn.\n");
            while (1) {
                ret = zc_socket_reconnect(sock);            
                if (ret < 0) {
                    ZCERROR("reconnect error:%s\n", strerror(-ret));
                }else{
                    ZCINFO("reconnect ok");
                    break;
                }
                sleep(1);
            }
            continue;
        }

        ZCINFO("recv %d: %s\n", ret, buf);
        sleep(1);
    }
    zc_socket_send(sock, "quit\r\n", 6);
    
    zc_socket_delete(sock);
    zc_socket_cleanup();

    return 0;
}
