#include "perf.h"
#include <getopt.h>
#include <pthread.h>
#include <string.h>
#include <locale.h>

#ifndef _WIN32
#include <xlocale.h>
#endif

#define MAX_THREADS 4096

/*char *thousand_number(uint64_t v, char *buf)
{
    char nbuf[256] = {0};

    snprintf(nbuf, sizeof(nbuf), "%llu", v);
}*/


void* run_single(void *args)
{
    TestParam *param = (TestParam*)args;
    param->stime = zc_timenow();
    zcHashTable *ht = zc_hashtable_new(1024);

    long i;
    int  ret;
    char buffer[4096] = {0};
    int  blen = 0; 
    if (param->conf.longconn) {
        zcSocket *sock = zc_socket_client_tcp("127.0.0.1", 10000, 5000);
        if (NULL == sock) {
            ZCERROR("connect error!");
            param->ret = -1;
            goto end;
        }
        zc_socket_tcp_nodelay(sock);
        //zc_socket_linger(sock, 1, 0);

        for (i=0; i<param->count; i++) {
            snprintf(buffer, sizeof(buffer), "%04ld\r\n", i);
            blen = strlen(buffer);
            ret = zc_socket_sendn(sock, buffer, blen);
            if (ret != blen) {
                ZCERROR("%ld send error:%d, %s\n", i, ret, buffer);
                param->ret = -2;
                goto end;
            }
            zc_hashtable_add(ht, buffer, 0, (void*)i);
            ret = zc_socket_recvn(sock, buffer, 4);
            if (ret != 4) {
                ZCERROR("%ld recv error:%d\n", i, ret);
                param->ret = -3;
                goto end;
            }
            buffer[ret] = 0;
            if (strcmp(buffer, "ok\r\n") != 0) {
                ZCERROR("%ld recv data error:%s\n", i, buffer);
                /*int n;
                for (n=0; n<4; n++) {
                    printf("%02x ", buffer[n]);
                }
                printf("\n");*/
                param->ret = -4;
                goto end;
            }
        }
        zc_socket_delete(sock);
    }else{
        for (i=0; i<param->count; i++) {
            zcSocket *sock = zc_socket_client_tcp("127.0.0.1", 10000, 1000);
            if (NULL == sock) {
                ZCERROR("connect error! 127.0.0.1:10000");
                param->ret = -1;
                goto end;
            }
            snprintf(buffer, sizeof(buffer), "%04ld\r\n", i);
            blen = strlen(buffer);
            ret = zc_socket_sendn(sock, buffer, blen);
            if (ret != blen) {
                ZCERROR("send error:%d, %s\n", ret, buffer);
                param->ret = -2;
                goto end;
            }
            ret = zc_socket_recvn(sock, buffer, 4);
            if (ret != 4) {
                ZCERROR("recv error:%d\n", ret);
                param->ret = -3;
                goto end;
            }
            buffer[ret] = 0;
            if (strcmp(buffer, "ok\r\n") != 0) {
                ZCERROR("recv data error:%s\n", buffer);
                /*int n;
                for (n=0; n<4; n++) {
                    printf("%02x ", buffer[n]);
                }
                printf("\n");*/
                param->ret = -4;
                goto end;
            }
            zc_socket_delete(sock);
        }
    }

end:
    param->etime = zc_timenow();

    return NULL;
}


void show_help()
{
    printf("usange:\n\tpref [options]\n\n");
    printf("\t--way,-w\tconcurrent way: thread/event\n");
    printf("\t--num,-n\trun number\n");
    printf("\t--longconn,-l\tuse long connection. default yes\n");
    printf("\t--client,-c\tusers of concurrent\n");
    printf("\t--input,-i\tinput file path\n");
    printf("\n\n");

    exit(0);
}

int main(int argc, const char * argv[])
{
    zc_log_new("stdout", ZC_LOG_ALL);

    setlocale(LC_ALL, "");

    int  optret;
    int  optidx = 0;
    const char *optstr = "w:n:l:c:i:";
    const struct option myoptions[] = {{"way", 0, NULL, 'w'},
                                 {"num", 0, NULL, 'n'},
                                 {"longconn", 0, NULL, 'l'},
                                 {"client", 0, NULL, 'c'},
                                 {"input", 0, NULL, 'i'}
                                 };

    if (argc < 2) {
        show_help();
        return 0;
    }

    Config conf;
    memset(&conf, 0, sizeof(Config));

    conf.way      = WAY_SINGLE;
    conf.num      = 100;
    conf.longconn = 0;
    conf.client   = 1;

    while (optret = getopt_long(argc, argv, optstr, myoptions, &optidx)) {
        if (0 > optret)
            break;
        switch (optret) {
        case 'w':
            if (strcmp(optarg, "thread") == 0 || strcmp(optarg, "t") == 0) {
                conf.way = WAY_THREAD;
            }else if (strcmp(optarg, "event") ==0 || strcmp(optarg, "e") == 0) {
                conf.way = WAY_EVENT;
            }else{
                printf("--way,-w must use thread/event! but you set a error value:%s\n", optarg);
                exit(0);
            }
            break;
        case 'n':
            conf.num = atoi(optarg);
            break;
        case 'l':
            if (strcmp(optarg, "yes") == 0 || strcmp(optarg, "y") == 0 || strcmp(optarg, "1") == 0) {
                conf.longconn = 1;
            }else{
                conf.longconn = 0;
            }
            break;
        case 'c':
            conf.client = atoi(optarg);
            break;
        case 'i':
            strcpy(conf.input, optarg);
        }
    }

    printf("====== config ======\n");
    printf("way:\t\t%d\n", conf.way);
    printf("num:\t\t%d\n", conf.num);
    printf("longconn:\t%d\n", conf.longconn);
    printf("client:\t\t%d\n", conf.client);
    printf("input:\t\t%s\n",  conf.input);

    TestParam *params[MAX_THREADS];

    uint64_t allstart = 0;
    int i;
    if (conf.client <= 1) {
        TestParam *param = (TestParam*)zc_malloc(sizeof(TestParam));
        memset(param, 0, sizeof(TestParam));
        param->id = 1;
        param->ctime = zc_timenow();
        memcpy(&param->conf, &conf, sizeof(conf));
        param->count = conf.num; 
        params[0] = param;

        allstart = zc_timenow();
        run_single(param);
    }else{
        if (conf.way == WAY_THREAD) {
            int ret;
            pthread_t threads[MAX_THREADS] = {0};

            allstart = zc_timenow();
            for (i=0; i<conf.client; i++) {
                TestParam *param = (TestParam*)zc_malloc(sizeof(TestParam));
                memset(param, 0, sizeof(TestParam));
                param->id = 1;
                param->ctime = zc_timenow();
                memcpy(&param->conf, &conf, sizeof(conf));
                param->count = conf.num/conf.client;
                params[i] = param;
                ret = pthread_create(&threads[i], NULL, run_single, param);
                if (ret != 0) {
                    char errbuf[128];
                    strerror_r(errno, errbuf, sizeof(errbuf));
                    ZCERROR("pthread create error! %s\n", errbuf);
                    return -1;
                }
            }

            for (i=0; i<conf.client; i++) {
                pthread_join(threads[i], NULL);
                int p = (i*10)/conf.client;
                if (p!=0 && (i*10)%conf.client == 0) {
                    ZCINFO("%d%% complete use %llu us\n", p*10, (unsigned long long)(zc_timenow()-allstart));
                }
            }
            ZCINFO("100%% complete use %llu us\n", (unsigned long long)(zc_timenow()-allstart));
        }else if (conf.way == WAY_EVENT) {
        }
    }
    uint64_t allend = zc_timenow();

    uint64_t timesum = 0;
    for (i=0; i<conf.client; i++) {
        timesum += (params[i]->etime - params[i]->stime);
    }

    //ZCINFO("start:%llu, end:%llu\n", allstart, allend);
    double speed = conf.num/((allend-allstart)/1000000.0);
    ZCINFO("== qps:%'.2f usetime:%'lluus resptime:%'.2fus ===\n", 
            speed, (unsigned long long)(allend-allstart), timesum/((double)conf.num));

    return 0;
}
