#include <stdio.h>
#include <assert.h>
#include <zocle/zocle.h>

int main()
{
    //zc_mem_init(ZC_MEM_GC|ZC_MEM_DBG_LEAK|ZC_MEM_DBG_OVERFLOW);
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_LEAK|ZC_MEM_DBG_OVERFLOW);
    //zc_mem_init(ZC_MEM_TCMALLOC);
    zc_log_new("stdout", ZC_LOG_ALL);

    unsigned int startid = zc_mem_check_point(0);
    ZCINFO("startid: %d\n", startid);
    int i;
    int count;
    char *a;
    char *addrs[10];

    count = 10;
    for (i = 0; i < count; i++) {
        a = zc_malloc(10);
        memset(a, 0, 11);
        assert(a != NULL);
        addrs[i] = a;
    }

    assert(zc_mem_count(startid) == count);
    assert(zc_check(a) != ZC_OK);

    startid = zc_mem_check_point(0);

    count = 5;
    for (i = 0; i < 5; i++) {
        a = zc_malloc(10);
        assert(a != NULL);
    }
    zc_mem_check_point(startid);

    ZCINFO("count: %d\n",  zc_mem_count(startid));
    assert(zc_mem_count(startid) == count);
    zc_free(a);
    ZCINFO("count: %d\n",  zc_mem_count(startid));
    assert(zc_mem_count(startid) == (count - 1));


    return 0;
}
