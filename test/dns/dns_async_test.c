#include <zocle/zocle.h>

int my_handle_ready(zcAsynConn *conn, char *data, int len)
{
    zcList *result = zc_list_new();
    result->del = zc_dnsrr_delete;
    int ret = zc_dns_unpack_resp_simple(result, data, len);
    
    ZCINFO("result size:%d, ret:%d", result->size, ret);
    zcDNSRR *r;
    zc_list_foreach(result, r) {
        zc_dnsrr_print(r);
    }
    ZCINFO("====== ok, delete ======");
    zc_list_delete(result);

    zc_asynconn_delete(conn);
    
    return ZC_OK;
}


int main(int argc, char *argv[])
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);
    zc_log_whole(_zc_log, 1);

    struct ev_loop *loop = ev_default_loop (0);

    zcAsynConn *conn = zc_asynconn_new_udp_client("202.106.0.20", 53, 5000, loop, 1024, 1024);
    if (NULL == conn) {
        ZCERROR("server create error");
        return 0;
    }
    
    int n = zc_dns_pack_query_simple(argv[1], ZC_DNS_T_A, ZC_DNS_C_IN, 
                conn->wbuf->data, zc_buffer_idle(conn->wbuf));
    ZCINFO("wbuf len:%d", n);
    conn->wbuf->end = n;
    zc_asynconn_start_write(conn);

    conn->handle_ready = my_handle_ready;

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
