#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <zocle/zocle.h>

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW); 
    zc_log_new("stdout", ZC_LOG_ALL);

    zcArray *ar;
    
    ar = zc_array_new(100, 4);
    assert(ar != NULL);
    assert(ar->items == 100);
    assert(ar->itemsize == 4);
    
    int i;
    for (i = 0; i < 100; i++) {
        zc_array_set(ar, i, (void*)&i);
        zc_check(ar);    
    }
    for (i = 0; i < 100; i++) {
        assert(*((int*)zc_array_get(ar, i, (void*)-1)) == i);
    }
    
    zc_check(ar);    
    zc_array_delete(ar);

    ar = zc_array_new(11, 5);
    char buf[6];
    for (i = 0; i < 10; i++) {
        sprintf(buf, "%05d", i);
        zc_array_set(ar, i, buf);
    }
    
    //zc_array_set(ar, 10, "abcde");

    for (i = 0; i < 10; i++) {
        sprintf(buf, "%05d", i);
        assert(memcmp(zc_array_get(ar, i, (void*)-1), buf, 5) == 0);
    }
    //assert(memcmp(zc_array_get(ar, 10, (void*)-1), "abcde", 5) == 0);
    
    ZCINFO("array size:%d\n", ar->items);
    ar = zc_array_expand(ar, 20);
    ZCINFO("array size:%d, itemsize:%d\n", ar->items, ar->itemsize);
    assert(ar->items == 31);
    assert(ar->itemsize == 5);

    for (i = 0; i < 10; i++) {
        sprintf(buf, "%05d", i);
        assert(memcmp(zc_array_get(ar, i, (void*)-1), buf, 5) == 0);
    }

    zc_array_delete(ar);

    return 0;
}
