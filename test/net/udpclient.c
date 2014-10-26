#include <zocle/zocle.h>

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);

    zcSocket    *sock;

    zc_socket_startup();
    sock = zc_socket_new_udp(0);
    if (NULL == sock) {
        ZCERROR("udp client create error!\n");
        return -1;
    }
    
    zc_socket_add_remote_addr(sock, "0.0.0.0", 10000);
    int ret;
    int i;
    for (i=0; i<10; i++) {
        char buf[1024];
        char sendbuf[128];
        sprintf(sendbuf, "ping%d", i);
        ret = zc_socket_sendto_self(sock, sendbuf, strlen(sendbuf), 0);
        if (ret != strlen(sendbuf)) {
            ZCERROR("send error! %d\n", ret);
            break;
        }
        //ZCINFO("recv ...");
        ret = zc_socket_recvfrom_self(sock, buf, 1024, 0);
        if (ret > 0) {
            buf[ret] = 0;
        }
        if (ret == 0) {
            ZCINFO("read 0, close conn.\n");
            break;
        }
        ZCINFO("recv %d: %s", ret, buf);
        //break;
        sleep(1);
    }
    
    zc_socket_delete(sock);
    zc_socket_cleanup();

    return 0;
}
