#include <zocle/zocle.h>

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);

    zcSocket *sock;
    sock = zc_socket_client_tcp("127.0.0.1", 10000, 10);
    if (NULL == sock) {
        ZCERROR("connect error!\n");
        return -1;
    }

    zc_check(sock);
    zcCmd *cmd = zc_cmd_new(sock, 4096, 10);

    zc_cmd_term_size(cmd, 4);

    while (1) {
        zcBuffer *wbuf = zc_cmd_wbuf(cmd); 

        zc_check(sock);
        char *data = "11111222223333344444";
        zc_buffer_set(wbuf, data, strlen(data));

        zc_check(sock);
        int ret;
        ret = zc_cmd_req(cmd);
        ZCINFO(">>> req: %d, %s\n", ret, data);
        if (ret < 0) {
            ZCERROR("req error:%d\n", ret);
            return -1;
        }

        zc_check(sock);
        ret = zc_cmd_resp(cmd);
        if (ret < 0) {
            ZCERROR("resp error:%d\n", ret);
            return -1;
        }
        zc_check(sock);
        zcBuffer *rbuf = zc_cmd_rbuf(cmd);
        ZCINFO("<<< resp:%d, %s\n", ret, rbuf->data);

        ZCINFO("====== sleep ======");
        sleep(1);
    }
    zc_check(sock);
    ZCINFO("====== destroy ======");
    zc_cmd_delete(cmd);
    zc_check(sock);
    zc_socket_delete(sock);
    

    return 0;
}
