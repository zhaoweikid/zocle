#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <zocle/zocle.h>

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW); 
    zc_log_new("stdout", ZC_LOG_ALL);

    zcAtomic at = zc_atomic_init(10);

    zc_atomic_inc(&at);
    assert(zc_atomic_read(&at) == 11);
    //ZCINFO("atomic: %d\n", zc_atomic_read(&at));

    zc_atomic_add(&at, 200);
    assert(zc_atomic_read(&at) == 211);
   
    zc_atomic_sub(&at, 100);
    assert(zc_atomic_read(&at) == 111);

    return 0;
}
