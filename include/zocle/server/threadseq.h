#ifndef ZOCLE_SERVER_THREADSEQ_H
#define ZOCLE_SERVER_THREADSEQ_H

#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <zocle/server/threadpool.h>

#define ZC_THREADSEQ_MAX   512

#define ZC_THREADSEQ_STOP	0
#define ZC_THREADSEQ_RUN	1

struct zc_threadseq_t;
typedef struct zc_threadseq_t zcThreadSeq;

/*typedef struct zc_threadqparam_t
{
    zcThreadSeq   *tq;
    zcThreadInfo  *info;
    void          *task;  
    int            pos;
}zcThreadQParam;*/

struct zc_threadseq_t
{
    uint16_t max_thread;
    uint16_t min_thread;
    uint16_t idle_thread;
    char     status;
    zcThreadInfo   *share;
    pthread_mutex_t lock;

    zcFuncRun produce;
    void      *args;
    zcFuncRun consume;
};

zcThreadSeq* zc_threadseq_new(int th);
void         zc_threadseq_delete(void*);
int          zc_threadseq_start(zcThreadSeq *q);
void*        zc_threadseq_run(void *args);
int          zc_threadseq_child(zcThreadSeq *q, int num);



#endif
