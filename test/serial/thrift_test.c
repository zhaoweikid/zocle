#include <zocle/zocle.h>
#include <assert.h>

void pack_test()
{


}

void unpack_test()
{
 

}

void perf_test()
{
    char *jsonstr = "{\"name\":\"zhaowei测试\",\"age\":18,\"data\":{\"msg\":\"haha\"},\"goods\":[{\"title\":\"test\"},{\"title\":\"test\"},{\"title\":\"test\"}]}";
    int64_t start, use;
    int i, ret;
    int count = 100000;
    zcString *jstr = zc_str_new_char(jsonstr, 0);
    zcObject *x = NULL;
    
    start = zc_timenow();
    for (i=0; i<count; i++) {
        ret = zc_json_unpack(&x, jstr);
        if (ret != ZC_OK) {
            ZCINFO("unpack error: %d, i:%d %s", ret, i, jstr->data);
        }
        assert(ret == ZC_OK);
        zc_obj_delete(x);
    }
    use = zc_timenow()-start;
    ZCINFO("unpack use time:%lld, qps:%.2f", (long long)use, ((float)count)/use * 1000000);


    zc_json_unpack(&x, jstr);
    assert(x != NULL);
    zcString *retstr = zc_str_new(1000);
    start = zc_timenow();
    for (i=0; i<count; i++) {
        ret = zc_json_pack(retstr, x);
        if (ret != ZC_OK) {
            ZCINFO("pack error: %d, i:%d %s", ret, i, jstr->data);
        }
        assert(ret == ZC_OK);
    }
    use = zc_timenow()-start;
    ZCINFO("pack use time:%lld, qps:%.2f", (long long)use, ((float)count)/use * 1000000);


    
} 
int main()
{
    //zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_LEAK|ZC_MEM_DBG_OVERFLOW);
    zc_mem_init(ZC_MEM_GLIBC);
    zc_log_new("stdout", ZC_LOG_ALL); 

    pack_test();
    unpack_test();
    perf_test();
    //check_test();

    return 0;
}

