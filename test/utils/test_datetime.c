#include <zocle/zocle.h>

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_INFO);
    
    zcDateTime dt;
    zc_datetime_init_now(&dt);
    ZCINFO("timstamp:%u", (unsigned int)dt.timestamp);

    zcDateTime dt2;
    zc_datetime_init_all(&dt2, 2013, 3, 28, 8, 46, 0, 0);
    ZCINFO("timstamp:%u", (unsigned int)dt2.timestamp);

    zcDateTime dt3;
    zc_datetime_init_str(&dt3, "2013-03-28 08:46:00 +0800");
    ZCINFO("timstamp:%u", (unsigned int)dt3.timestamp);


    return 0;
}
