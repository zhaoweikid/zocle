#include <zocle/zocle.h>

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);

    zcSocket    *sock;
    //ZCINFO("zcSocket size:%d, zcSockAddr size:%d", sizeof(zcSocket), sizeof(zcSockAddr));
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
        char buf[1024];
        ZCINFO("new socket remote %s:%d, local %s:%d\n", 
                    newsock->remote.ip, newsock->remote.port,
                    newsock->local.ip, newsock->local.port);
        while (1) {
            ret = zc_socket_recv(newsock, buf, 1024);
            buf[ret] = 0;

            if (ret == 0) {
                ZCINFO("read 0, close conn.\n");
                zc_socket_delete(newsock);
                break;
            }

            ZCINFO("recv %d: %s\n", ret, buf);
            if (strncmp(buf, "quit", 4) == 0) {
                ZCINFO("quit");
                zc_socket_delete(newsock);
                break;
            }

            ret = zc_socket_sendn(newsock, "ok\r\n", 4);
            if (ret != 4) {
                ZCERROR("send error:%d\n", ret);
                zc_socket_delete(newsock);
                break;
            }
        }
    }
    
    zc_socket_delete(sock);
    zc_socket_cleanup();

    return 0;
}
