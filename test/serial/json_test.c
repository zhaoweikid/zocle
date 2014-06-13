#include <zocle/zocle.h>
#include <assert.h>

void pack_test()
{
    zcHashTable *ht = zc_hashtable_new(1024, 0);
    
    zc_hashtable_add_str(ht, "test", zc_str_new_char("haha", 0));
    zc_hashtable_add_str(ht, "test2", zc_str_new_char("haha2是什么", 0));

    zcInt8 n8 = ZC_INT8_INIT(10);
    zc_hashtable_add_str(ht, "test3", &n8);

    zcInt64 n64 = ZC_INT64_INIT(1231890238917239817);
    zc_hashtable_add_str(ht, "test4", &n64);

    zcBool nb = ZC_BOOL_INIT(true);
    zc_hashtable_add_str(ht, "test5", &nb);

    zcNull nn = ZC_NULL_INIT();
    zc_hashtable_add_str(ht, "test6", &nn);

    zcDouble nd = ZC_DOUBLE_INIT(12345.4567);
    zc_hashtable_add_str(ht, "test8", &nd);

    zcList *list = zc_list_new();
    zc_list_append(list, zc_str_new_char("gogo我看看", 0));
    zc_list_append(list, zc_str_new_char("换行\n行不", 0));
    zc_list_append(list, zc_str_new_char("引号呢\"行不", 0));
    zc_list_append(list, zc_str_new_char("反斜杠呢\\行不", 0));
    zc_list_append(list, zc_str_new_char("正斜杠呢/行不", 0));
    zc_list_append(list, zc_str_new_char("Tab呢\t行不", 0));

    zc_hashtable_add_str(ht, "mylist", list);

    zcString *s = zc_str_new(128);

    zc_json_pack(s, (zcObject*)ht);
    
    char *j = "{\"test\":\"haha\",\"test2\":\"haha2是什么\",\"test3\":10,\"test4\":1231890238917239817,\"test5\":true,\"test6\":null,\"test8\":12345.456700,\"mylist\":[\"gogo我看看\",\"换行\\n行不\",\"引号呢\\\"行不\",\"反斜杠呢\\\\行不\",\"正斜杠呢\\/行不\",\"Tab呢\\t行不\"]}";

    ZCINFO("data:%s\n", s->data);
    ZCINFO("   j:%s\n", j);

    assert(strcmp(s->data, j) == 0);
}

void unpack_test()
{
    int ret;
    zcObject *x = NULL;
    
    ZCINFO("====== double ======");
    zcString *json1 = zc_str_new_char("-123.45", 0);
    ret = zc_json_unpack(&x, json1);
    if (ret != 0) {
        ZCERROR("json unpack error:%d", ret);
        return;
    }
    ZCINFO("double:%f", ((zcDouble*)x)->v);
    zc_free(x);
   
    // ------ string
    ZCINFO("====== string ======");
    zc_str_clear(json1);
    zc_str_append(json1, "\"ab\tc我们haha\"");
    ret = zc_json_unpack(&x, json1);
    if (ret != 0) {
        ZCERROR("json unpack error:%d", ret);
        return;
    }
    ZCINFO("string:%s", ((zcString*)x)->data);
    zc_free(x);
    
    // ------ null
    ZCINFO("====== null ======");
    zc_str_clear(json1);
    zc_str_append(json1, "null");
    ret = zc_json_unpack(&x, json1);
    if (ret != 0) {
        ZCERROR("json unpack error:%d", ret);
        return;
    }
    ZCINFO("null:%d, %d", x->__type,  ZC_NULL);
    zc_free(x);
 
    // ------ true
    ZCINFO("====== bool true ======");
    zc_str_clear(json1);
    zc_str_append(json1, "true");
    ret = zc_json_unpack(&x, json1);
    if (ret != 0) {
        ZCERROR("json unpack error:%d", ret);
        return;
    }
    ZCINFO("bool:%d, %d", x->__type,  ((zcBool*)x)->v);
    zc_free(x);
 
    // ------ false
    ZCINFO("====== bool false ======");
    zc_str_clear(json1);
    zc_str_append(json1, "false");
    ret = zc_json_unpack(&x, json1);
    if (ret != 0) {
        ZCERROR("json unpack error:%d", ret);
        return;
    }
    ZCINFO("bool:%d, %d", x->__type,  ((zcBool*)x)->v);
    zc_free(x);
 
    // ------ list
    ZCINFO("====== list ======");
    zc_str_clear(json1);
    zc_str_append(json1, "[\"aaa\",\"234\"]");
    ZCINFO("test list:%s", json1->data);
    ret = zc_json_unpack(&x, json1);
    if (ret != 0) {
        ZCERROR("json unpack error:%d", ret);
        return;
    }
    zcList *list = (zcList*)x;
    ZCINFO("list size:%d", list->size);
    zcString *s;
    zc_list_foreach(list, s) {
        ZCINFO("list string:%d, %s", x->__type,  s->data);
    }

    assert(list->size == 2);

    s = zc_list_at(list, 0, NULL);
    ZCINFO("list first:%s", s->data);
    assert(s != NULL);
    assert(strcmp(s->data, "aaa") == 0);

    s = zc_list_at(list, 1, NULL);
    ZCINFO("list second:%s", s->data);
    assert(s != NULL);
    assert(strcmp(s->data, "234") == 0);

    zc_free(x);
 
    // ------ dict
    ZCINFO("====== dict ======");
    zc_str_clear(json1);
    zc_str_append(json1, "{\"aaa\":\"bcb测试cbc\",\"234\":\"33就是3333333\"}");
    ZCINFO("test dict:%s", json1->data);
    ret = zc_json_unpack(&x, json1);
    if (ret != 0) {
        ZCERROR("json unpack error:%d", ret);
        return;
    }
    zcHashTable *ht = (zcHashTable*)x;
    ZCINFO("dict size:%d", ht->size);
    assert(ht->len == 2);

    const char *key;
    zcString *value;

    value = zc_hashtable_get_str(ht, "aaa", NULL);
    assert(value != NULL);
    assert(strcmp(value->data, "bcb测试cbc") == 0);

    value = zc_hashtable_get_str(ht, "234", NULL);
    assert(value != NULL);
    assert(strcmp(value->data, "33就是3333333") == 0);

    zc_hashtable_foreach_start(ht, key, value)
        ZCINFO("list key:%s, value:%s", key,  value->data);
    zc_hashtable_foreach_end
    zc_obj_delete(x);
 
    //unsigned int startid = zc_mem_check_point(0);
    //ZCINFO("====== object ======");
    zc_str_clear(json1);
    /*int64_t size = zc_file_size("test.json");
    zc_str_ensure_idle_size(json1, size+1); 
    
    FILE *fp;
    fp = fopen("test.json", "r+");
    ret = fread(json1->data, 1, size, fp);
    ZCINFO("fread ret:%d", ret);
    json1->len = ret;
    fclose(fp);*/

    ZCINFO("====== unpack performance ======");
    zc_str_append(json1, "{\"ret\":0, \"msg\":\"错误信息\", \"data\":[{\"name\":\"zhaowei\"}, {\"name\":\"bobo\"}], \"data2\":[{\"name\":\"zhaowei1\"}, {\"name\":\"bobo1\"}], \"money\":123.17, \"lnglat\":[12.22, 453.09]}");

    ZCINFO("test json:%s", json1->data);

    int64_t start = zc_timenow();
    int i;
    int count = 50000;
    for (i=0; i<count; i++) {
        ret = zc_json_unpack(&x, json1);
        if (ret != 0) {
            ZCERROR("json unpack error:%d", ret);
            return;
        }
        //ht = (zcHashTable*)x;
        //ZCINFO("dict size:%d", ht->size);
        //zc_obj_print(x);
        zc_obj_delete(x);
    }

    int64_t use = zc_timenow()-start;
    ZCINFO("unpack use time:%lld, qps:%.2f", (long long)use, ((float)count)/use * 1000000);
 
    //zc_mem_check_point(startid); 
    //ZCINFO("count: %d\n",  zc_mem_count(startid));
    
    ZCINFO("====== pack performance ======");

    ret = zc_json_unpack(&x, json1);
    if (ret != 0) {
        ZCERROR("json unpack error: %d", ret);
        return;
    }

    start = zc_timenow();
    zcString *js = zc_str_new(1024); 
    for (i=0; i<count; i++) {
        ret = zc_json_pack(js, x);
        if (ret < 0) {
            ZCERROR("json pack error:%d", ret);
            return;
        }
    }
    use = zc_timenow()-start;
    ZCINFO("pack use time:%lld, qps:%.2f", (long long)use, ((float)count)/use * 1000000);
 

}

void check_test()
{
    const char *j[] = {"", "[", "[]", "{}", "[1,a,b]", "{1:\"b\",\"c\":\"x}", NULL};
    int i = 0;
    int ret;
    zcString s;

    /*char *a = "我们";
    printf("a:%s\n", a);

    for (i=0; i<strlen(a); i++) {
        printf("i:%d %x, ", i, *(a+i));
    }
    printf("\n");

    char b[128] = {0};
    ret = zc_iconv_convert("utf-8", "ucs2", a, strlen(a), b, sizeof(b));
    ZCINFO("convert:%s");*/

    while (j[i] != NULL) {
        zc_str_init_stack(&s, (char*)j[i], strlen(j[i]));
        ret = zc_json_check(&s);
        assert(ret!=ZC_OK);
        i++;
    }

}

int main()
{
    //zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_LEAK|ZC_MEM_DBG_OVERFLOW);
    zc_mem_init(ZC_MEM_GLIBC);
    zc_log_new("stdout", ZC_LOG_ALL); 

    pack_test();
    unpack_test();
    //check_test();

    return 0;
}

