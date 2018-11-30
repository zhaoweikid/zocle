#include <zocle/zocle.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void check_key(zcDict *ht)
{
    char *k, *v;
    zc_dict_foreach_start(ht, k, v)
        //ZCINFO("check key:%s\n", k);
        zc_check(k);
        zc_check(v);
    zc_dict_foreach_end
    //zc_check(ht->table);
    zc_check(ht);
}


int test_dict1()
{
    zcDict   *ht;

    ht = zc_dict_new(100);
    assert(ht != NULL);

    ht->keydel = zc_free_func;
    ht->valdel = zc_free_func;

    int  i;
    int  ret;
    char key[64];
    char val[64];
    int  count = 100;

    for (i = 0; i < count; i++) {
        sprintf(key, "k%d", i);
        sprintf(val, "value%d", i);
        ret = zc_dict_add(ht, key, 0, zc_strdup(val,0));
        //ZCINFO("key: %s, ret: %d\n", key, ret);
        assert(ret == ZC_OK);
    }

    char *k, *v;
    zc_dict_foreach_start(ht,  k, v)
        ZCINFO("key:%s, val:%s\n", k, v);
    zc_dict_foreach_end

    for (i = 0; i < count; i++) {
        sprintf(key, "k%d", i);
        //ZCINFO("haskey:%s\n", key);
        assert(zc_dict_haskey(ht, key, 0) == ZC_TRUE);
    }

    for (i = count; i < count + 200; i++) {
        sprintf(key, "k%d", i);
        assert(zc_dict_haskey(ht, key, 0) != ZC_TRUE);
    }

    //zc_dict_print(ht);
    for (i = 0; i < count; i++) {
        sprintf(key, "k%d", i);
        ret = zc_dict_rm(ht, key, 0);
        //ZCINFO("rm key:%s, %d\n", key, ret);
        assert(ret == ZC_OK);
    }

    for (i = count; i < count + 200; i++) {
        sprintf(key, "k%d", i);
        assert(zc_dict_rm(ht, key, 0) != ZC_OK);
    }

    zc_dict_clear(ht);
    assert(ht->len == 0);

    for (i = 0; i < count; i++) {
        sprintf(key, "k%d", i);
        sprintf(val, "value%d", i);
        ret =zc_dict_add(ht, key, 0, zc_strdup(val,0));
        //ZCINFO("key: %s, ret: %d\n", key, ret);
        assert(ret == ZC_OK);
    }

    for (i = 0; i < count; i++) {
        sprintf(key, "k%d", i);
        assert(zc_dict_haskey(ht, key, 0) == ZC_TRUE);
    }

    int oldsize, oldlen;
    oldsize = ht->size;
    oldlen  = ht->len;

    ZCINFO("resize to 10000 ... %d\n", ht->size);
    assert(zc_dict_resize(ht, 10000) == ZC_OK);
    zc_check(ht);
    //zc_check(ht->table);

    check_key(ht);

    assert(ht->size >= 10000);
    assert(ht->len == oldlen);

    for (i = 0; i < count; i++) {
        sprintf(key, "k%d", i);
        assert(zc_dict_haskey(ht, key, 0) == ZC_TRUE);
    }

    //zc_dict_print(ht);
    zc_dict_delete(ht);

    return 0;
}


int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);

    test_dict1();

    return 0;
}

