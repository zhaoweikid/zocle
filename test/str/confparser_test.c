#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <assert.h>
#include <zocle/zocle.h>

typedef struct
{
    int     count;
    float   value;
    char    ip[16];
    short   port;
    int     ischeck;
    int     ids[10];
    char    name[128];
}Conf;

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
                    "include test2.conf",
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

int check_data(Conf *cf)
{
    ZCINFO("count:%d\n", cf->count);
    ZCINFO("value:%f %e %e\n", cf->value, fabs(cf->value - 11.23), FLT_EPSILON);
    ZCINFO("ip:%s\n", cf->ip);
    ZCINFO("port:%d\n", cf->port);
    ZCINFO("ischeck:%d\n", cf->ischeck);

    ZCINFO("ids[0]:%d\n", cf->ids[0]);
    ZCINFO("ids[1]:%d\n", cf->ids[1]);
    ZCINFO("ids[2]:%d\n", cf->ids[2]);
    ZCINFO("name:%s\n", cf->name);

    assert(cf->count == 100);
    assert(fabs(cf->value - 11.23) <= 0.000001);
    assert(strcmp(cf->ip, "127.0.0.1") == 0);
    assert(cf->port == 3366);
    assert(cf->ischeck == 1);
    
    assert(cf->ids[0] == 11);
    assert(cf->ids[1] == 33);
    assert(cf->ids[2] == 455);

    assert(strcmp(cf->name, "zhaowei") == 0);

    return 0;
}

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);
    ZCINFO("go!\n");
    init_data();

    Conf *cf = zc_malloc(sizeof(Conf));
    memset(cf, 0, sizeof(Conf));
   
    zcConfParser *p = zc_confparser_new("test.conf");
    
    zc_confparser_add(p, &cf->count, "count", ZC_CONF_INT);
    zc_confparser_add(p, &cf->value, "value", ZC_CONF_FLOAT);
    zc_confparser_add(p, cf->ip, "ip", ZC_CONF_STRING);
    zc_confparser_add(p, &cf->port, "port", ZC_CONF_INT);
    zc_confparser_add(p, &cf->ischeck, "ischeck", ZC_CONF_BOOL);
    zc_confparser_add_array(p, &cf->ids, "ids", ZC_CONF_INT, 10);
    zc_confparser_add(p, cf->name, "name", ZC_CONF_STRING);

    zcList *servers = zc_list_new();
    zc_list_set_del(servers, zc_free_func);
    zc_confparser_add_array(p, servers, "server", ZC_CONF_STRING, 10);
    
    zc_confparser_parse(p);
    zc_confparser_delete(p);
    
    ZCINFO("server count:%d\n", servers->size);
    char *data;
    zc_list_foreach(servers, data) {
        ZCINFO("server:%s\n", data);
    }

    check_data(cf); 
    
    return 0;
}
