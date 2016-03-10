#include <zocle/zocle.h>

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_INFO);

    ZCINFO("test timer ...");
    //zcTimer *tm = zc_timer_new();

    zcTimer tm;
    zc_timer_start(&tm);
    ZCINFO("sleep 1");
    sleep(1);
    unsigned int ret = zc_timer_end(&tm);
    ZCINFO("ret:%u\n", ret);
    sleep(3);
    ret = zc_timer_end(&tm);
    ZCINFO("ret:%u\n", ret);

    uint64_t x = zc_timenow();

    ZCINFO("timenow:%lu\n", x);

    return 0;
}
