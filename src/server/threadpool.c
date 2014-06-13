#include <zocle/server/threadpool.h>
#include <zocle/base/defines.h>
#include <zocle/mem/alloc.h>
#include <zocle/ds/queue.h>
#include <zocle/log/logfile.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

zcThreadPool* 
zc_threadpool_new(int qsize, int prcnum, int maxthread, int minthread, int idle)
{
    zcThreadPool  *tp;
    tp = (zcThreadPool*)zc_malloc(sizeof(zcThreadPool));
    memset(tp, 0, sizeof(zcThreadPool));

    tp->consumer.max_thread  = maxthread;
    tp->consumer.min_thread  = minthread;
    tp->consumer.idle_thread = idle;
    tp->consumer.inc_step    = 1;
    tp->consumer.status      = ZC_THREADPOOL_RUN;

    tp->producer.max_thread  = tp->producer.min_thread = prcnum;
    tp->producer.idle_thread = 0;
    tp->producer.inc_step    = 1;
    tp->producer.status      = ZC_THREADPOOL_RUN;

    //tp->proc_info = (zProcShare*)zc_malloc(sizeof(zcProcShare));
    //tp->queue_max_size = qsize;

    tp->queue = zc_queue_new(qsize);
    if (NULL == tp->queue) {
        goto threadpool_new_error;
    }

    tp->consumer.share = (zcThreadInfo*)zc_malloc(sizeof(zcThreadInfo) * tp->consumer.max_thread);
    if (NULL == tp->consumer.share) {
        goto threadpool_new_error;
    }
    memset(tp->consumer.share, 0, sizeof(zcThreadInfo)*tp->consumer.max_thread);

    tp->producer.share = (zcThreadInfo*)zc_malloc(sizeof(zcThreadInfo)*tp->producer.max_thread);
    if (NULL == tp->producer.share) {
        goto threadpool_new_error;
    }
    memset(tp->producer.share, 0, sizeof(zcThreadInfo)*tp->producer.max_thread);

    int ret;
    ret = pthread_mutex_init(&tp->producer.lock, NULL);
    if (ret != 0) {
        goto threadpool_new_error;   
    }
    ret = pthread_mutex_init(&tp->consumer.lock, NULL);
    if (ret != 0) {
        pthread_mutex_destroy(&tp->producer.lock);
        goto threadpool_new_error;   
    }
    ret = pthread_mutex_init(&tp->lock, NULL);
    if (ret != 0) {
        pthread_mutex_destroy(&tp->consumer.lock);
        goto threadpool_new_error;   
    }
    return tp;

threadpool_new_error:
    if (tp->queue) {
        zc_queue_delete(tp->queue);
    }
    //if (tp->proc_info) {
    //    zc_free(tp->proc_info);
    //}
    if (tp->consumer.share) {
        zc_free(tp->consumer.share);
    }
    if (tp->producer.share) {
        zc_free(tp->producer.share);
    }
    if (tp) {
        zc_free(tp);
    }
    return NULL;
}

void  
zc_threadpool_delete(void *x)
{
    zcThreadPool *tp = (zcThreadPool*)x;
    zc_free(tp->producer.share);
    zc_free(tp->consumer.share);

    zc_queue_delete(tp->queue);

    pthread_mutex_destroy(&tp->lock);
    pthread_mutex_destroy(&tp->consumer.lock);
    pthread_mutex_destroy(&tp->producer.lock);

    zc_free(tp);
}

int   
zc_threadpool_child(zcThreadPool *tp, zcThreadSetting *ts, int num)
{
    int i, c;
    int ret;
    pthread_t tid;
    zcThreadInfo *base, *share;
    zcFuncRun run;

    if (ts == &tp->consumer) {
        run = zc_threadpool_consumer_run;
    }else{
        run = zc_threadpool_producer_run;
    }

    if (num <= 0) {
        ZCWARN("create child num error:%d", num);
        return ZC_OK;
    }
    ZCINFO("create thread num:%d", num);
    base = ts->share;
    c = 0;
    for (i=0; i<ts->max_thread; i++) {
        share = base + i;
        if (share->tid <= 0) {
            ret = pthread_create(&tid, NULL, run, tp);
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
            ZCINFO("start thread %llu %d ...\n", (unsigned long long)pthread_self(), c);
            c++;
            if (c == num) {
                break;
            }
        }
    }
    return ZC_OK;
}

void* 
zc_threadpool_run(zcThreadPool *tp, zcThreadSetting *ts)
{
    zcThreadInfo *base, *share;
    int i;
    int thread_pos;
    void *task = NULL;
    const char *name;

    if (ts == &tp->producer) {
        name = "producer";
    }else{
        name = "consumer";
    }

    if (NULL == tp) {
        ZCERROR("%s thread is NULL", name);
        return NULL;
    }
    base = ts->share;
    pthread_mutex_lock(&ts->lock);
    for (i=0; i<ts->max_thread; i++) {
        share = base + i;
        // 分配一个可用的线程索引号
        if (share->tid <= 0) {
            share->tid = pthread_self();
            thread_pos = i;
            break;
        }
    }
    ts->threads++;
    pthread_mutex_unlock(&ts->lock);

    if (i == ts->max_thread) {
        ZCERROR("%s thread id error!", name);
        return NULL;
    }

    share->create_time = time(NULL);
    share->start_time  = 0;
    share->perform     = 0;
    share->concur      = 0;
    share->status      = ZC_THREADPOOL_RUN;
    share->user_data   = NULL;

    int  idles = 0; // 线程累积连续空闲次数
    void *defv = "<default>";

    if (&tp->producer == ts) {
        while (ts->status == ZC_THREADPOOL_RUN) {
            share->start_time = time(NULL);
            share->concur += 1;

            zcThreadParam param;
            param.threadpool = tp;
            param.setting    = ts;
            param.task       = task;
            param.info       = share;
            param.pos        = thread_pos;
            ts->run(&param);

            share->concur -= 1;
            share->perform++;
        }
    }else{
        while (share->status == ZC_THREADPOOL_RUN) {
            int queuelen = zc_queue_length(tp->queue);
            if ((0 == queuelen) && (ZC_THREADPOOL_STOP == ts->status)){
                ZCWARN("%s exit", name);
                pthread_exit(NULL);
            }else if ((queuelen == tp->queue->size) &&
                    (ts->threads < ts->max_thread)) { // 创建新的线程
                int n = (ts->max_thread-ts->threads > ts->inc_step)? \
                        ts->inc_step : ts->max_thread - ts->threads;
                zc_threadpool_child(tp, ts, n);
            }
            task = zc_queue_get(tp->queue, true, 5, defv);
            if (task == defv) {
                idles++;
                // 如果有连续10次空闲，则退出，相当于连续50秒空闲, 当然线程数不能小于最小线程数
                if ((idles > 10) && (ts->threads > ts->min_thread)){
                    share->tid = 0;
                    pthread_mutex_lock(&ts->lock);
                    ts->threads--;
                    pthread_mutex_unlock(&ts->lock);
                    ZCWARN("%s exit", name);
                    pthread_exit(NULL);
                }
                continue;
            }

            idles = 0;
            share->start_time = time(NULL);
            share->concur += 1;

            zcThreadParam param;
            param.threadpool = tp;
            param.setting    = ts;
            param.task       = task;
            param.info       = share;
            param.pos        = thread_pos;
            ts->run(&param);

            share->concur -= 1;
            share->perform++;
        }
    }
    
    ZCWARN("thread end");
    return NULL;
}

void* 
zc_threadpool_consumer_run(void *args)
{
    zcThreadPool *tp = (zcThreadPool*)args;
    return zc_threadpool_run(tp, &tp->consumer);
}

void* 
zc_threadpool_producer_run(void *args)
{
    zcThreadPool *tp = (zcThreadPool*)args;
    return zc_threadpool_run(tp, &tp->producer);
}

void  
zc_threadpool_start(zcThreadPool *tp)
{
    if (NULL == tp->consumer.run) {
        ZCERROR("consumer function must not NULL\n");
        return;
    }
    ZCINFO("create producer count:%d\n", tp->producer.min_thread);
    zc_threadpool_child(tp, &tp->producer, tp->producer.min_thread);

    ZCINFO("create consumer count:%d\n", tp->consumer.min_thread);
    zc_threadpool_child(tp, &tp->consumer, tp->consumer.min_thread-1);
    zc_threadpool_run(tp, &tp->consumer);

    pthread_exit(NULL);
}

/*void  
zc_threadpool_manage(zcThreadPool *tp)
{
    while (1) {
        sleep(10);
        ZCINFO("pid:%u, create:%u, start:%u, perform:%d, concur:%d\n", 
            tp->proc_info->pid, tp->proc_info->create_time,
            tp->proc_info->start_time, tp->proc_info->perform, 
            tp->proc_info->concur);
    }
}*/



