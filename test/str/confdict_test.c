#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <assert.h>
#include <zocle/zocle.h>

int init_data()
{
    char *items[] = {
                    "count=100\n", 
                    "value=11.23\n", 
                    "ip=  127.0.0.1 \n",
                    "port=3366\n",
                    "ischeck=yes\n",
                    "ids=11,33,455\n",
                    "server=127.0.0.1,192.168.1.1,192.168.1.100\n",
                    "testserver=127.0.0.1:8000,\n\t127.0.0.1:9000,\n\t127.0.0.1:9999\n",
                    NULL};

    unlink("test.conf");
    FILE *fp = fopen("test.conf", "w");
    int i = 0;

    while (items[i] != NULL) {
        fwrite(items[i], 1, strlen(items[i]), fp);
        i++;
    }

    fclose(fp);

    return 0;
}


int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);
    init_data();

    zcConfDict  *conf = (zcConfDict*)zc_confdict_new("test.conf"); 
    int ret;
    ret = zc_confdict_parse(conf);
    if (ret != ZC_OK) {
        ZCINFO("parse error! %d", ret);
        return -1;
    }
    zc_confdict_print(conf);    
    ZCINFO("count: %d", zc_confdict_get_int(conf, NULL, "count", 0));
    assert(zc_confdict_get_int(conf, NULL, "count", 0) == 100);

    int values[32] = {0};
    ret = zc_confdict_get_array_int(conf, NULL, "ids", values, sizeof(values)/sizeof(int));
    ZCINFO("ret:%d", ret);
    if (ret > 0) {
        int i;
        for (i=0; i<ret; i++) {
            ZCINFO("ids %d: %d", i, values[i]);
        }
    }


    zcList *list = zc_confdict_get_list(conf, NULL, "server");
    if (list) {
        char *value;
        zcListNode *node;
        zc_list_foreach(list, node) {
            value = (char*)node->data;
            ZCINFO("value:%s", value);
        }
    }

    zc_confdict_delete(conf);


    return 0;
}
