#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <zocle/zocle.h>

int test1()
{
    zcArray *ar;

    ar = zc_array_new(100);
    assert(ar != NULL);
    assert(ar->len == 0);
    assert(ar->cap == 100);

    long i = 0;
    //zc_array_set_many(ar, 100, (void*)i);

    for (i = 0; i < 100; i++) {
        //zc_array_set(ar, i, (void*)i);
        zc_array_append(ar, (void*)i);
        zc_check(ar);
    }
    for (i = 0; i < 100; i++) {
        long geti = (long)zc_array_get(ar, i, (void*)-1);
        //ZCINFO("i:%ld, %ld", i, geti);
        assert(geti == i);
    }

    for (i=0; i<50; i++) {
        zc_array_set(ar, i, (void*)(i*100));
    }
    for (i = 0; i < 50; i++) {
        long geti = (long)zc_array_get(ar, i, (void*)-1);
        //ZCINFO("i:%ld, %ld", i, geti);
        assert(geti == i*100);
    }

    for (i=0; i<10; i++) {
        assert(zc_array_append(ar, (void*)i) == ZC_OK);
    }
    for (i=100; i<110; i++) {
        long geti = (long)zc_array_get(ar, i, (void*)-1);
        //ZCINFO("i:%ld, %ld", i, geti);
        assert(geti == i-100);
    }


    zc_check(ar);
    zc_array_delete(ar);


    return 0;
}

int test2()
{
    zcArray *ar;

    ar = zc_array_new_tail(100);
    assert(ar != NULL);
    assert(ar->len == 0);
    assert(ar->cap == 100);

    long i = 0;
    //zc_array_set_many(ar, 100, (void*)i);

    for (i = 0; i < 100; i++) {
        //zc_array_set(ar, i, (void*)i);
        zc_array_append(ar, (void*)i);
        zc_check(ar);
    }
    for (i = 0; i < 100; i++) {
        long geti = (long)zc_array_get(ar, i, (void*)-1);
        //ZCINFO("i:%ld, %ld", i, geti);
        assert(geti == i);
    }

    for (i=0; i<50; i++) {
        zc_array_set(ar, i, (void*)(i*100));
    }
    for (i = 0; i < 50; i++) {
        long geti = (long)zc_array_get(ar, i, (void*)-1);
        //ZCINFO("i:%ld, %ld", i, geti);
        assert(geti == i*100);
    }

    for (i=0; i<10; i++) {
        assert(zc_array_append(ar, (void*)i) == ZC_ERR);
    }

    zc_check(ar);
    zc_array_delete(ar);

    return 0;
}

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);

    test1();
    test2();

    return 0;
}
