#ifndef ZOCLE_SERVER_THREADQUEUE_H
#define ZOCLE_SERVER_THREADQUEUE_H

#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <zocle/server/threadpool.h>

#define ZC_THREADQUEUE_MAX   512

#define ZC_THREADQUEUE_STOP	0
#define ZC_THREADQUEUE_RUN	1

struct zc_threadqueue_t;
typedef struct zc_threadqueue_t zcThreadQueue;

typedef struct zc_threadqparam_t
{
    zcThreadQueue *tq;
    zcThreadInfo  *info;
    void          *task;  
    int            pos;
}zcThreadQParam;

struct zc_threadqueue_t
{
    uint16_t max_thread;
    uint16_t min_thread;
    uint16_t idle_thread;
    char status;
    zcThreadInfo   *share;
    pthread_mutex_t lock;

    zcFuncRun produce;
    zcFuncRun consume;
};

zcThreadQueue* zc_threadqueue_new(int th);
void           zc_threadqueue_delete(void*);
int            zc_threadqueue_start(zcThreadQueue *q);
void*          zc_threadqueue_run(void *args);
int            zc_threadqueue_child(zcThreadQueue *q, int num);



#endif
