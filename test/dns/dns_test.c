#include <zocle/zocle.h>

int main(int argc, char *argv[])
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);
    zc_log_whole(_zc_log, 1);

    zcList *list = zc_list_new();
    list->del = zc_dnsrr_delete;
    int ret = zc_dns_query("202.106.0.20", argv[1], ZC_DNS_T_A, ZC_DNS_C_IN, list);
    ZCINFO("query ret:%d", ret);

    zc_list_delete(list);

    return 0;
}
