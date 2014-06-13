#include <zocle/zocle.h>

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);

    zcSocket    *sock;

    zc_socket_startup();
    
    sock = zc_socket_server_udp("127.0.0.1", 10000, 0);
    if (NULL == sock) {
        ZCERROR("udp server create error!\n");
        return -1;
    }

    ZCINFO("udp server start ok!\n");
    int ret;
    while (1) {
        char buf[1024];
        ret = zc_socket_recvfrom_self(sock, buf, 1024, 0);
        if (ret > 0){
            buf[ret] = 0;
        }else {
            ZCERROR("recvfrom error! %d\n", ret);
            break;
        }
        ZCINFO("recv %d: %s\n", ret, buf);

        char *resp = "pong!\r\n";
        ret = zc_socket_sendto_self(sock, resp, strlen(resp), 0);
        if (ret != 7) {
            ZCERROR("sendto error! %d\n", ret);
            break;
        }
        //break;
    }
    
    zc_socket_delete(sock);
    zc_socket_cleanup();

    return 0; 
}

