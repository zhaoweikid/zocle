#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <zocle/zocle.h>

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW); 
    zc_log_new("stdout", ZC_LOG_ALL);

    zcArrayPtr *ar;
    
    ar = zc_arrayptr_new(100);
    assert(ar != NULL);
    assert(ar->items == 100);
    
    int i;
    char *a[100];
    for (i = 0; i < 100; i++) {
        char *s = (char*)malloc(32);
        sprintf(s, "v%d", i);
        a[i] = s;
    }

    for (i = 0;  i < 100; i++) {
        zc_arrayptr_set(ar, i, a[i]);
    }

    void* retv;
    for (i = 100; i < 200; i++) {
        retv = zc_arrayptr_get(ar, i, (void*)-1);
        assert(retv == (void*)-1);
    }
   
    for (i = 0; i < 100; i++) {
        retv = zc_arrayptr_get(ar, i, (void*)-1);
        //ZCINFO("retv:%p, %s\n", retv, (char*)retv);
        assert(retv == a[i]);

        free(a[i]);
    }

    zc_check(ar);    
    zc_arrayptr_delete(ar);

    return 0;
}
