#include <zocle/zocle.h>

int myhttp_ready(zcAsynConn *conn)
{
    zcHttpInfo *info = (zcHttpInfo*)conn->data;
    ZCINFO("ready data:%s", info->resp->bodydata.data);    

    return ZC_OK;
}

int test1(const char *url)
{
    struct ev_loop *loop = ev_default_loop (0);

    zcHttpInfo *info = zc_httpinfo_new();
    info->readyfunc = myhttp_ready;
    zcAsynConn *conn = zc_asynconn_new_http_url(url, 30000, loop, "202.106.0.20", 65535, info);
    if (NULL == conn) {
        ZCERROR("server create error");
        return 0;
    }
    
    zc_check(conn);
    zc_check(conn->sock);
    zc_check(conn->rbuf);
    zc_check(conn->wbuf);
   
    ZCINFO("started");
    ev_run (loop, 0);
    ZCINFO("stopped");
    
    return ZC_OK;
}

int myhttp_read(const char *data, int datalen, bool finish, void *x)
{
    zcAsynConn *conn = (zcAsynConn*)x;
    zcHttpInfo *info = (zcHttpInfo*)conn->data;

    ZCINFO("readlen:%lld", info->readlen);
    if (info->readlen == info->resp->bodylen || datalen == 0) {
        ZCINFO("read end!");
    }

    FILE *f = fopen("c.dat", "a+");
    fwrite(data, 1, datalen, f);
    fclose(f);

    return ZC_OK;
}

int test2(const char *url)
{
    struct ev_loop *loop = ev_default_loop (0);

    zcHttpInfo *info = zc_httpinfo_new();
    info->readyfunc = myhttp_ready;
    info->readfunc  = myhttp_read;
    zcAsynConn *conn = zc_asynconn_new_http_url(url, 30000, loop, "202.106.0.20", 65535, info);
    if (NULL == conn) {
        ZCERROR("server create error");
        return 0;
    }
    
    zc_check(conn);
    zc_check(conn->sock);
    zc_check(conn->rbuf);
    zc_check(conn->wbuf);
   
    ZCINFO("started");
    ev_run (loop, 0);
    ZCINFO("stopped");
    
    return ZC_OK;
}


int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("usage: httpasync_test url\n");
        return 0;
    }

    char url[1024] = {0};
    if (strncmp(argv[1], "http", 4) == 0) {
        strcpy(url, argv[1]);
    }else{
        strcpy(url, "http://");
        strcpy(url+7, argv[1]);
    }

    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);
    zc_log_whole(_zc_log, 1);

    test1(url);
    //test2(url);

    return 0;
}
