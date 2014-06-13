#ifndef _WIN32
#include <zocle/zocle.h>

int runner(zcProc *p)
{
    char buf[1024] = {0};
    zcSocket *sock = (zcSocket*)p->userdata;
    while (1) {
        ZCNOTE("accept at:%d ...", sock->fd);
        zcSocket *newsock = zc_socket_accept(sock);
        if (NULL == newsock) {
            ZCNOTE("accept error, continue");
            continue;
        }
        ZCINFO("accept %d", newsock->fd);
        while (1) {
            int ret = zc_socket_recv(newsock, buf, sizeof(buf));
            if (ret <= 0) {
                ZCNOTE("recv error! %d, %s", ret, strerror(-1*ret));
                zc_socket_delete(newsock);
                break;
            }
            buf[ret] = 0;

            //ZCINFO("recv:%s", buf); 
            strcpy(buf, "ok\r\n");
            ret = zc_socket_send(newsock, buf, strlen(buf));
            if (ret != 4) {
                ZCNOTE("send error! %d, %s\n", ret, strerror(-1*ret));
                zc_socket_delete(newsock);
                break;
            }
        }
        //zc_socket_delete(newsock);
    }

    return 0;
}

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);

    zcSocket *sock = zc_socket_server_tcp("127.0.0.1", 10000, 32);
    if (NULL == sock) {
        ZCERROR("socket create error!");
        return -1;
    }

    ZCNOTE("ok, go");
    zcProc  *proc = zc_proc_new(10,1,1,100000,2000000);
    if (NULL == proc) {
        ZCERROR("proc create error!");
        return -1;
    }
    proc->run = runner;
    proc->userdata = sock;
    
    zc_proc_run(proc);

    return 0;
}

#else
int main(){return 0;}
#endif
