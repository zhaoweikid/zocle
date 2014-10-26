#include <zocle/zocle.h>

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);

#ifdef ZOCLE_WITH_SSL
    zcSocket    *sock;

    zc_socket_startup();

    sock = zc_socket_new_tcp(10);
    if (NULL == sock) {
        ZCERROR("tcp client create error!\n");
        return -1;
    }
    
    int ret;

    ret = zc_socket_connect(sock, "127.0.0.1", 10000);
    if (ret != ZC_OK) {
        ZCINFO("connect 127.0.0.1:9000 error!\n"); 
        return -1;
    }

    ret = zc_socket_client_ssl(sock, NULL, NULL, ZC_SSL_CERT_NONE, ZC_SSL_VER_SSL3, NULL);
    if (ret != ZC_OK) {
        ZCERROR("ssl error:%d\n", ret);
        return -1;
    }
    ZCINFO("create ssl ok!\n"); 
    /*ret = zc_socket_connect(sock, "127.0.0.1", 9000);
    if (ret != ZC_OK) {
        ZCINFO("connect 127.0.0.1:9000 error!\n"); 
        return -1;
    }*/

    ZCINFO("socket remote:%s:%d, local:%s:%d\n", 
            sock->remote.ip, sock->remote.port,
            sock->local.ip, sock->local.port);
    int i;
    for (i=0; i<10; i++) {
        char buf[1024];
        char sendbuf[128];
        sprintf(sendbuf, "good-%d\r\n", i);
        
        ret = zc_socket_send(sock, sendbuf, strlen(sendbuf));
        if (ret != strlen(sendbuf)) {
            ZCERROR("send error! %d\n", ret);
            break;
        }
        ret = zc_socket_recv(sock, buf, 1024);
        if (ret == 0) {
            ZCINFO("recv close");
            break;
        }
        if (ret > 0) {
            buf[ret] = 0;
        }
        ZCINFO("recv %d: %s\n", ret, buf);
    }
    
    zc_socket_delete(sock);
    zc_socket_cleanup();
#endif


    return 0;
}
