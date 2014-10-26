#include <zocle/zocle.h>

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);

#ifdef ZOCLE_WITH_SSL
    zcSocket    *sock;

    zc_socket_startup();

    sock = zc_socket_server_tcp("127.0.0.1", 10000, 32);
    if (NULL == sock) {
        ZCERROR("tcp server create error!\n");
        return -1;
    }

    ZCINFO("tcp server start ok!\n");
    int ret;
    while (1) {
        zcSocket *newsock = zc_socket_accept(sock);
        if (NULL == newsock)
            break;

        ret = zc_socket_server_ssl(newsock, "cert.pem", "cert.pem", ZC_SSL_CERT_NONE, ZC_SSL_VER_SSL3, NULL);
        if (ret != ZC_OK) {
            ZCERROR("tcp ssl error! %d\n", ret);
            return -1;
        }

        char buf[1024];
        ZCINFO("new socket remote:%s:%d, local:%s:%d\n", 
                    newsock->remote.ip, newsock->remote.port,
                    newsock->local.ip, newsock->local.port);
        char sendbuf[128];
        while (1) {
            zc_check(newsock);
            ret = zc_socket_recv(newsock, buf, 1024);
            if (ret == 0) {
                ZCINFO("read 0, close conn.\n");
                //zc_socket_delete(newsock);
                break;
            }
            buf[ret] = 0;

            ZCINFO("recv %d: %s\n", ret, buf);
            sprintf(sendbuf, "good!\r\n");
            ret = zc_socket_send(newsock, sendbuf, strlen(sendbuf));
            if (ret != strlen(sendbuf)) {
                ZCINFO("send error:%d", ret);
                break;
            }
            ZCINFO("send:%s", sendbuf);
        }
        zc_check(newsock);
        zc_socket_delete(newsock);
    }
    
    zc_socket_delete(sock);
    zc_socket_cleanup();
#endif

    return 0;
}
