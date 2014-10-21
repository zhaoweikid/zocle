#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <zocle/zocle.h>

int test()
{
    zcArray *ar;
    
    ar = zc_array_new(100);
    assert(ar != NULL);
    assert(ar->len == 0);
    assert(ar->cap == 100);
   
    ZCINFO("set 100 ...");
    long i;
    for (i = 0; i < 100; i++) {
        zc_array_set(ar, i, (void*)i);
        zc_check(ar);    
    }
    ZCINFO("get 100 ...");
    for (i = 0; i < 100; i++) {
        assert(*((int*)zc_array_get(ar, i, (void*)-1)) == i);
    }
    
    zc_check(ar);    
    ZCINFO("delete");
    zc_array_delete(ar);


    return 0;
}

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW); 
    zc_log_new("stdout", ZC_LOG_ALL);

    test();

    return 0;
}
