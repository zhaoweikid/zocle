#include <zocle/zocle.h>

int test1(const char *myurl)
{
    ZCINFO("============ test1 =============");
    zcHttpReq  *r = zc_httpreq_new(myurl);
    zc_url_print(&r->url);
    
    zcList *urls = zc_list_new();
    zc_socket_gethostbyname(r->url.domain.data, urls);

    //int i;
    ZCINFO("urls: %d", urls->size);
    zcListNode *node;
    zc_list_foreach(urls, node) {
        ZCINFO("ip: %s", (char*)node->data);
    }
    
    zcHttpConnStat stat;
    memset(&stat, 0, sizeof(stat));


    //ZCINFO("connect %s:80", zc_cstrlist_get(&urls, 0));
    //zcHttpConn *c = zc_httpconn_new(zc_cstrlist_get(&urls, 0), 80); 
    zcHttpConn *c = zc_httpconn_new(); 
    c->stat = &stat;
    int ret = zc_httpconn_send(c, r);
    if (ret != ZC_OK) {
        ZCERROR("send error: %d", ret);
        return 0;
    }

    zcHttpResp *resp = zc_httpconn_recv(c);
    if (NULL == resp) {
        ZCERROR("recv error!");
        return 0;
    }

    ZCINFO("header:%s", resp->headdata.data);
    //ZCINFO("body:%d", resp->bodydata.len);
    ZCINFO("body:%d %s", resp->bodydata.len, resp->bodydata.data);
    ZCINFO("================================================="); 
    /*for (i=0; i<resp->bodydata.len; i++) {
        printf("%c", resp->bodydata.data[i]);
    }*/


    zc_httpconnstat_print(&stat);

    ZCINFO("delete httpresp");
    zc_httpresp_delete(resp);
    ZCINFO("delete httpreq");
    zc_httpreq_delete(r);
    ZCINFO("delete httpconn");
    zc_httpconn_delete(c);

    return 0;
}

int myrecv(const char *s, int len)
{
    //ZCINFO("<<< recv: %d >>>", len);
    FILE *f = fopen("b.html", "a+");
    fwrite(s, 1, len, f);
    fclose(f);
    return ZC_OK;
}


int test2(const char *myurl)
{
    ZCINFO("============ test2 =============");
    system("rm -rf b.html");

    zcHttpConnStat stat;
    memset(&stat, 0, sizeof(stat));

    zcHttpReq  *r = zc_httpreq_new(myurl);
    zc_url_print(&r->url);
    
    zcList *urls = zc_list_new();
    zc_socket_gethostbyname(r->url.domain.data, urls);

    //int i;
    ZCINFO("urls: %d", urls->size);
    zcListNode *node;
    zc_list_foreach(urls, node) {
        ZCINFO("ip: %s", (char*)node->data);
    }
 
    //ZCINFO("connect %s:80", zc_cstrlist_get(&urls, 0));
    //zcHttpConn *c = zc_httpconn_new(zc_cstrlist_get(&urls, 0), 80); 
    zcHttpConn *c = zc_httpconn_new(); 
    c->readfunc = myrecv;
    c->stat = &stat;

    int ret;
    //ZCINFO("stat:%p", c->stat);
    ret = zc_httpconn_send(c, r);
    if (ret != ZC_OK) {
        ZCERROR("send error: %d", ret);
        zc_httpconnstat_print(&stat);
        return 0;
    }

    zcHttpResp *resp = zc_httpconn_recv(c);
    if (NULL == resp) {
        ZCERROR("recv error!");
        zc_httpconnstat_print(&stat);
        return 0;
    }

    ZCINFO("header:%s", resp->headdata.data);
    //ZCINFO("body:%d", resp->bodydata.len);
    //ZCINFO("body:%d %s", resp->bodydata.len, resp->bodydata.data);
    zc_httpconnstat_print(&stat);
    
    
    zc_httpresp_delete(resp);
    //ZCINFO("delete httpreq");
    zc_httpreq_delete(r);
    //ZCINFO("delete httpconn");
    zc_httpconn_delete(c);

    return 0;
}

int test3(const char *myurl, zcHttpConn *c)
{
    ZCINFO("================= longconn again ========================"); 
    int ret;
    zcHttpConnStat stat;
    memset(&stat, 0, sizeof(stat));

    zcHttpReq  *r = zc_httpreq_new(myurl);
    zc_url_print(&r->url);

    c->stat = &stat;
    memset(c->stat, 0, sizeof(zcHttpConnStat));
    ret = zc_httpconn_send(c, r);
    if (ret != ZC_OK) {
        ZCERROR("send error: %d", ret);
        return 0;
    }

    zcHttpResp *resp = zc_httpconn_recv(c);
    if (NULL == resp) {
        ZCERROR("recv error!");
        return 0;
    }
    ZCINFO("body:%d %s", resp->bodydata.len, resp->bodydata.data);
    zc_httpconnstat_print(&stat);

    //ZCINFO("delete httpreq");
    zc_httpreq_delete(r);
    //ZCINFO("delete httpconn");
    zc_httpconn_delete(c);

    return 0;
}

int main(int argc, char *argv[])
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);
    zc_log_whole(_zc_log, 1);
    
    if (argc != 2) {
        ZCFATAL("args: url");
        return -1;
    }
    char url[1024] = {0};
    if (strncmp(argv[1], "http", 4) == 0) {
        strcpy(url, argv[1]);
    }else{
        strcpy(url, "http://");
        strcpy(url+7, argv[1]);
    }
    zc_socket_startup();

    test1(url);
    //test2(url);
    //test3(url);

    zc_socket_cleanup();

    return 0;
}

