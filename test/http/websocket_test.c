#include <zocle/zocle.h>

int myhttp_ready(zcAsynConn *conn)
{
    zcHttpInfo *info = (zcHttpInfo*)conn->data;
    ZCINFO("ready data:%d", info->resp->bodydata.len);    

    /*int ret = zc_httpresp_check_websocket(info->resp, info->req);
    if (ret == ZC_OK) {
        ZCINFO("websocket ok!, go!");
        zc_asynhttp_websocket(conn);
    }else{
        ZCINFO("websocket error! %d", ret);
    }*/

    return ZC_OK;
}

int sendreq(zcAsynConn *conn, void *data)
{
    char text[65535] = {0}; 
   
    int i;
    for (i=0; i<256; i++) {
        text[i] = 'a';
    }
    ZCINFO("send request:%s", text);
    zc_asynhttp_websocket_send(conn, 1, ZC_WS_TEXT, text, strlen(text));

    return ZC_OK;
}


int myhttp_read(const char *data, int datalen, bool finish, void *x)
{
    ZCINFO("ok, in readfunc");

    zcAsynConn *conn = (zcAsynConn*)x;
    zcHttpInfo *info = (zcHttpInfo*)conn->data;

    char buffer[65535] = {0};
    memcpy(buffer, data, datalen);

    ZCINFO("readlen:%lld, datalen:%d, %s", info->readlen, datalen, buffer);
    if (datalen == 0) {
        ZCINFO("read end!");
    }else if (datalen > 0) {
        FILE *f = fopen("c.dat", "a+");
        fwrite(data, 1, datalen, f);
        fclose(f);
    }
    
    ZCINFO("call after 1000ms\n-----------------------------------------\n");
    zc_asynconn_call_later(conn, 100, 0, sendreq, NULL);

    return ZC_OK;
}

int test(const char *url)
{
    struct ev_loop *loop = ev_default_loop (0);

    zcHttpInfo *info = zc_httpinfo_new();
    info->readyfunc = myhttp_ready;
    info->readfunc  = myhttp_read;
    zcAsynConn *conn = zc_asynconn_new_http_url(url, 3000, loop, "202.106.0.20", 500000, info);
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
    if (strncmp(argv[1], "ws://", 5) == 0) {
        strcpy(url, argv[1]);
    }else{
        strcpy(url, "ws://");
        strcat(url, argv[1]);
    }

    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);
    zc_log_whole(_zc_log, 1);

    test(url);

    return 0;
}
