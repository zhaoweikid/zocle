#include <zocle/zocle.h>

int main(int argc, char *argv[])
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);
    zc_log_whole(_zc_log, 1);

    //zcHttpReq *req = zc_httpreq_new();
    //zcHttpClient *cli = zc_httpclient_new();

    return 0;
}
