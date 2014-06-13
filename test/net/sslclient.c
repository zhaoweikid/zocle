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

    ret = zc_socket_connect(sock, "127.0.0.1", 9000);
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

    while (1) {
        char buf[1024];
        
        ret = zc_socket_send(sock, "good!\r\n", 7);
        if (ret != 7) {
            ZCERROR("send error! %d\n", ret);
            break;
        }
        ret = zc_socket_recv(sock, buf, 1024);
        ZCINFO("recv %d: %s\n", ret, buf);
        if (ret > 0) {
            buf[ret] = 0;
        }
        if (ret == 0) {
            break;
        }

        break;
    }
    
    zc_socket_delete(sock);
    zc_socket_cleanup();
#endif


    return 0;
}
