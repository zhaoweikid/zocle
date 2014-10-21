#include <zocle/zocle.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int test_queue1()
{
    return 0;
}


int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW); 
    zc_log_new("stdout", ZC_LOG_ALL);
    
    test_queue1();

    return 0;
}

