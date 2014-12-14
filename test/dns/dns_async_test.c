#include <zocle/zocle.h>

int my_handle_read(zcAsynConn *conn)
{
    zcBuffer *rbuf = conn->rbuf;
    char *data = zc_buffer_data(rbuf);
    int len = zc_buffer_used(rbuf);

    ZCINFO("buffer len:%d", len);

    zcList *result = zc_list_new();
    result->del = zc_dnsrr_delete;
    int ret = zc_dns_unpack_resp_simple(result, data, len);
    
    ZCINFO("result size:%d, ret:%d", result->size, ret);
    zcListNode *node;
    zcDNSRR *r;
    zc_list_foreach(result, node) {
        r = (zcDNSRR*)node->data;
        zc_dnsrr_print(r);
    }
    //ZCINFO("====== ok, delete ======");
    zc_list_delete(result);

    //zc_asynconn_delete(conn);
    return len; 
}

int test1(char *host)
{
    struct ev_loop *loop = ev_default_loop (0);

    zcProtocol p;
    zc_protocol_init(&p);
    p.handle_read = my_handle_read;
    zcAsynConn *conn = zc_asynconn_new_udp_client("202.106.0.20", 53, 5000, &p, loop, 1024, 1024);
    if (NULL == conn) {
        ZCERROR("server create error");
        return 0;
    }
    
    int n = zc_dns_pack_query_simple(host, ZC_DNS_T_A, ZC_DNS_C_IN, 
                conn->wbuf->data, zc_buffer_idle(conn->wbuf));
    ZCINFO("wbuf len:%d", n);
    conn->wbuf->end = n;
    zc_asynconn_start_write(conn);

    //conn->handle_ready = my_handle_ready;

    zc_check(conn);
    zc_check(conn->sock);
    zc_check(conn->rbuf);
    zc_check(conn->wbuf);
   
    ZCINFO("started");
    ev_run (loop, 0);
    ZCINFO("stopped");
    
    zc_asynconn_delete(conn);
 
    return 0;
}


int test2(char *host)
{
    struct ev_loop *loop = ev_default_loop (0);

    zcAsynConn *conn = zc_asynconn_new_dns_client("202.106.0.20", 5000, loop, host, my_handle_read);
    if (NULL == conn) {
        ZCERROR("server create error");
        return 0;
    }
    
    ZCINFO("started");
    ev_run (loop, 0);
    ZCINFO("stopped");
    
    zc_asynconn_delete(conn);
 
    return 0;
}

int main(int argc, char *argv[])
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);
    zc_log_whole(_zc_log, 1);

    test2(argv[1]);

    return 0;
}
