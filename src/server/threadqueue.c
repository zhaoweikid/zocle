#include <zocle/server/threadqueue.h>
#include <zocle/log/logfile.h>
#include <zocle/base/defines.h>
#include <zocle/mem/alloc.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

zcThreadQueue*   
zc_threadqueue_new (int th)
//zc_threadqueue_new (int maxth, int minth, int idle)
{
    zcThreadQueue    *q; 

    q = (zcThreadQueue*)zc_malloc(sizeof(zcThreadQueue));
    memset(q, 0, sizeof(zcThreadQueue));

    q->max_thread  = th;
    q->min_thread  = th;
    q->idle_thread = 0;
    q->status      = ZC_THREADQUEUE_RUN;

    q->share = (zcThreadInfo*)zc_malloc(sizeof(zcThreadInfo) * q->max_thread);
    if (NULL == q->share) {
        zc_free(q);
        return NULL;
    }   

    int ret = pthread_mutex_init(&q->lock, NULL);
    if (ret != 0) {
        ZCERROR("pthread_mutex_init error");
        zc_free(q->share);
        zc_free(q);
        return NULL;
    }

    return q;
}

void    
zc_threadqueue_delete (void *x)
{
    zcThreadQueue *q = (zcThreadQueue*)x;
    pthread_mutex_destroy(&q->lock);
    zc_free(q->share);
    zc_free(q);
}

int
zc_threadqueue_start (zcThreadQueue *q)
{
    if (NULL == q->consume) {
        ZCWARN("consume must not null");
        return ZC_ERR;
    }

    zc_threadqueue_child(q, q->min_thread-1);
    zc_threadqueue_run(q);

    return ZC_OK;
}


int
zc_threadqueue_child (zcThreadQueue *q, int num)
{
    int i, c;
    int ret;
    pthread_t tid;
    zcThreadInfo *base, *share;

    if (num <= 0) {
        ZCWARN("thread create num:%d", num);
        return ZC_OK;
    }

    base = q->share;
    c = 0;
    pthread_mutex_lock(&q->lock);
    for (i=0; i<q->min_thread; i++) {
        share = base + i;
        if (share->tid <= 0) {
            ret = pthread_create(&tid, NULL, zc_threadqueue_run, q);
            if (0 != ret) {
                ZCERROR("create thread faild. %d, %d\n", i, errno);
                exit(-1);
                return ZC_ERR;
            }
            ret = pthread_detach(tid);
            if (0 != ret) {
                ZCERROR("set thread detach faild. %d, %d\n", i, errno);
                exit(-1);
                return ZC_ERR;
            }
            c++;
            if (c == num) {
                break;
            }
        }
    }
    pthread_mutex_unlock(&q->lock);
    return ZC_OK;
}

void*
zc_threadqueue_run(void *args)
{
    zcThreadQueue *q = args;
    zcThreadInfo *base, *share;
    int   threadpos;
    int   i;
    
    ZCINFO("start");
    base = q->share;
    pthread_mutex_lock(&q->lock);
    for (i = 0; i < q->max_thread; i++) {
        share = base + i;
        if (share->tid <= 0) {
            share->tid = pthread_self();
            threadpos = i;
            break;
        }
    }
    pthread_mutex_unlock(&q->lock);

    if (i == q->max_thread) {
        ZCERROR("thread id create error!\n");
        return NULL;
    }

    share->create_time = time(NULL);
    share->start_time  = 0;
    share->perform     = 0;
    share->concur      = 0;
    share->status      = ZC_THREADQUEUE_RUN;
    share->user_data   = NULL;

    zcThreadQParam param;
    param.tq   = q;
    param.info = share;
    param.pos  = threadpos;

    void *ret = NULL;
    while (1) {
        if (q->status == ZC_THREADQUEUE_STOP) {
            ZCWARN("thread exit");
            pthread_exit(NULL);
        }
        ret = NULL;
        if (q->produce) {
            pthread_mutex_lock(&q->lock);
            ret = q->produce(&param);
            pthread_mutex_unlock(&q->lock);
            
            if (NULL != ret) {
                q->consume(&param);
            }
        }else{
            q->consume(&param);
        }
    }
    
    ZCWARN("thread end");
    return NULL;
}
