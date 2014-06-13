#include <zocle/zocle.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int test_lhashtable1()
{
    zcLHashTable  *ht;

    ht = zc_lhashtable_new(1000);
    assert(ht != NULL);

    ht->keydel = zc_free_func;
    ht->valdel = zc_free_func;
    
    int i;
    int ret;
    char key[64];
    char val[64];
    int count = 1000;

    for (i = 0; i < count; i++) {
        sprintf(key, "k%d", i);
        sprintf(val, "value%d", i);
        //zcLHashTableNode *n = (zcLHashTableNode*)node;
        //void *nk = (char*)n + sizeof(zcLHashTableNodeBase);
        //ZCINFO("base size:%d\n", sizeof(zcLHashTableNodeBase));
        //ZCINFO("node key:%s, val:%s, n:%p, %p, %p, %s\n", (char*)n->key, (char*)n->value, n, nk, *(char**)nk, *(char**)nk);
        ret = zc_lhashtable_add(ht, key, 0, val);

        //ZCINFO("key: %s, ret: %d\n", key, ret);
        assert(ret == ZC_OK);
    }

    for (i = 0; i < count; i++) {
        sprintf(key, "k%d", i);
        assert(zc_lhashtable_haskey(ht, key, 0) == ZC_TRUE);
    }
 
    for (i = count; i < count + 200; i++) {
        sprintf(key, "k%d", i);
        assert(zc_lhashtable_haskey(ht, key, 0) != ZC_TRUE);
    }

    //zc_lhashtable_print(ht);
    for (i = 0; i < count; i++) {
        sprintf(key, "k%d", i);
        ret = zc_lhashtable_rm(ht, key, 0);
        //ZCINFO("rm key:%s, %d\n", key, ret);
        assert(ret == ZC_OK);
    }

    for (i = count; i < count + 200; i++) {
        sprintf(key, "k%d", i);
        assert(zc_lhashtable_rm(ht, key, 0) != ZC_OK);
    }

    zc_lhashtable_clear(ht);
    assert(ht->len == 0);

    for (i = 0; i < count; i++) {
        sprintf(key, "k%d", i);
        sprintf(val, "value%d", i);
        ret =zc_lhashtable_add(ht, key, 0, val);
        //ZCINFO("key: %s, ret: %d\n", key, ret);
        assert(ret == ZC_OK);
    }

    for (i = 0; i < count; i++) {
        sprintf(key, "k%d", i);
        assert(zc_lhashtable_haskey(ht, key, 0) == ZC_TRUE);
    }
 
    int oldsize, oldlen;
    oldsize = ht->size;
    oldlen  = ht->len;

    ZCINFO("resize to 10000 ...\n");
    assert(zc_lhashtable_resize(ht, 10000) == ZC_OK);
    assert(ht->size == 10000);
    assert(ht->len == oldlen);

    for (i = 0; i < count; i++) {
        sprintf(key, "k%d", i);
        assert(zc_lhashtable_haskey(ht, key, 0) == ZC_TRUE);
    }
   
    //zc_lhashtable_print(ht);
    
    zc_lhashtable_delete(ht);

    return 0;
}


int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW); 
    zc_log_new("stdout", ZC_LOG_ALL);
    
    test_lhashtable1();

    return 0;
}

