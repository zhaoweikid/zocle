#include <zocle/zocle.h>
#include <string.h>
#include <assert.h>


int test_hash()
{
    zcHash   *hs;

    hs = zc_hash_new(1000, 0);
    assert(hs != NULL);
    
    int i, ret;
    char buf[64];

    // add key: 0-100
    for (i = 0; i < 100; i++) {
        sprintf(buf, "%d", i);
        assert(zc_hash_add(hs, zc_strdup(buf, strlen(buf)), strlen(buf)) == ZC_OK);
    }

    for (i = 0; i < 100; i++) {
        sprintf(buf, "%d", i);
        assert(zc_hash_haskey(hs, buf, strlen(buf)) == ZC_TRUE);
    }
 
    for (i = 100; i < 200; i++) {
        sprintf(buf, "%d", i);
        assert(zc_hash_haskey(hs, buf, strlen(buf)) != ZC_TRUE);
    }

    // rm key: 0-100
    for (i = 0; i < 100; i++) {
        sprintf(buf, "%d", i);
        assert(zc_hash_rm(hs, buf, strlen(buf)) == ZC_OK);
    }

    for (i = 100; i < 200; i++) {
        sprintf(buf, "%d", i);
        assert(zc_hash_rm(hs, buf, strlen(buf)) != ZC_OK);
    }

    zc_hash_clear(hs);
    assert(hs->len == 0);
    
    // add key:0-1000
    for (i = 0; i < 1000; i++) {
        sprintf(buf, "%d", i);
        ret = zc_hash_add(hs, zc_strdup(buf, strlen(buf)), strlen(buf));
        /*ZCINFO("add key:%s, %d\n", buf, ret);
        if (i == 259) {
            zc_hash_print(hs);
        }*/
        assert(ret == ZC_OK);
    }

    for (i = 0; i < 1000; i++) {
        sprintf(buf, "%d", i);
        assert(zc_hash_haskey(hs, buf, strlen(buf)) == ZC_TRUE);
    }
 
    int oldsize, oldlen;
    oldsize = hs->size;
    oldlen  = hs->len;

    ZCINFO("resize to 10000 ...\n");
    assert(zc_hash_resize(hs, 10000) == ZC_OK);
    assert(hs->size >= 10000);
    assert(hs->len == oldlen);

    for (i = 0; i < 1000; i++) {
        sprintf(buf, "%d", i);
        assert(zc_hash_haskey(hs, buf, strlen(buf)) == ZC_TRUE);
    }
    
    zc_hash_delete(hs);
 
    return 0;
}


int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW); 
    zc_log_new("stdout", ZC_LOG_ALL);
    
    test_hash();

    return 0;
}

