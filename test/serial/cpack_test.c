#include <stdio.h>
#include <assert.h>
#include <zocle/zocle.h>

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_LEAK|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL); 

    zcString *buf = zc_str_new(128);

    int v1 = 21;
    char v2[4] = {1, 2, 3, 4};
    int len = zc_cpack(buf, "$4ic4", v1, v2);
    ZCINFO("len:%d", len);
    zc_str_delete(buf);

    return 0;
}
