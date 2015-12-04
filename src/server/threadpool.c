#include <zocle/server/threadpool.h>
#include <zocle/base/defines.h>
#include <zocle/mem/alloc.h>
#include <zocle/ds/queue.h>
#include <zocle/log/logfile.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int   zc_threadpool_child(zcThreadPool*, zcThreadSetting*, int);
void* zc_threadpool_run(void*);

zcThreadPool* 
zc_threadpool_new(int qsize, int maxthread, int minthread, int idle, zcFuncRun run)
{
    zcThreadSetting setting;
    
    memset(&setting, 0, sizeof(zcThreadSetting));
    setting.max_thread  = maxthread;
    setting.min_thread  = minthread;
    setting.idle_thread = idle;
    setting.inc_step    = 1;
    setting.run         = run;
    setting.status      = ZC_THREADPOOL_RUN;
    
    return zc_threadpool_new_setting(qsize, &setting);
}

zcThreadPool* 
zc_threadpool_new_setting(int qsize, zcThreadSetting *setting) 
{
    zcThreadPool  *tp;
    tp = (zcThreadPool*)zc_malloc(sizeof(zcThreadPool));
    memset(tp, 0, sizeof(zcThreadPool));

    tp->queue = zc_queue_new(qsize);
    if (NULL == tp->queue) {
        goto threadpool_new_error;
    }

    tp->share = (zcThreadInfo*)zc_malloc(sizeof(zcThreadInfo) * tp->setting.max_thread);
    if (NULL == tp->share) {
        goto threadpool_new_error;
    }
    memset(tp->share, 0, sizeof(zcThreadInfo)*tp->setting.max_thread);

    memcpy(&tp->setting, setting, sizeof(zcThreadSetting));

    int ret;
    ret = pthread_mutex_init(&tp->lock, NULL);
    if (ret != 0) {
        goto threadpool_new_error;   
    }
    return tp;

threadpool_new_error:
    if (tp->queue) {
        zc_queue_delete(tp->queue);
    }
    if (tp->share) {
        zc_free(tp->share);
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
    zc_free(tp->share);

    zc_queue_delete(tp->queue);

    pthread_mutex_destroy(&tp->lock);

    zc_free(tp);
}

int   
zc_threadpool_child(zcThreadPool *tp, zcThreadSetting *ts, int num)
{
    int i, c;
    int ret;
    pthread_t tid;
    zcThreadInfo *base, *share;
    //zcFuncRun run;

    //run = zc_threadpool_setting_run;

    if (num <= 0) {
        ZCWARN("create child num error:%d", num);
        return ZC_OK;
    }
    ZCINFO("create thread num:%d", num);
    base = tp->share;
    c = 0;
    for (i=0; i<ts->max_thread; i++) {
        share = base + i;
        if (share->tid <= 0) {
            ret = pthread_create(&tid, NULL, zc_threadpool_run, tp);
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
zc_threadpool_run(void *args)
{
    zcThreadInfo *base, *share;
    int i;
    zcThreadPool *tp = (zcThreadPool*)args;
    zcThreadSetting *ts = &tp->setting;
    zcTask *task = NULL;

    base = tp->share;
    pthread_mutex_lock(&tp->lock);
    for (i=0; i<ts->max_thread; i++) {
        share = base + i;
        if (share->tid <= 0) {
            share->tid = pthread_self();
            //thread_pos = i;
            break;
        }
    }
    pthread_mutex_unlock(&tp->lock);
                    

    if (i >= ts->max_thread) {
        ZCERROR("thread out of number");
        return NULL;
    }
    
    __sync_add_and_fetch(&tp->threads, 1);

    share->create_time = time(NULL);
    share->start_time  = 0;
    share->perform     = 0;
    share->concur      = 0;
    share->status      = ZC_THREADPOOL_RUN;
    share->user_data   = NULL;

    int  idles = 0; // 线程累积连续空闲次数
    void *defv = "<default>";

    while (share->status == ZC_THREADPOOL_RUN) {
        int queuelen = zc_queue_size(tp->queue);
        if ((0 == queuelen) && (ZC_THREADPOOL_STOP == ts->status)){
            share->tid = 0;
            __sync_sub_and_fetch(&tp->threads, 1);
            ZCWARN("thread exit, found stop status");
            pthread_exit(NULL);
            return NULL;
        }else if ((queuelen == tp->queue->size) &&
                (tp->threads < ts->max_thread)) { // queue full, create new thread
            int n = (ts->max_thread - tp->threads > ts->inc_step)? \
                    ts->inc_step : ts->max_thread - tp->threads;
            zc_threadpool_child(tp, ts, n);
        }
        task = (zcTask*)zc_queue_get(tp->queue, true, 5000, defv);
        if (task == defv) { // no task
            idles++;
            // 如果有连续10次退出，相当于连续50秒空闲, 线程数不能小于最小线程数
            if ((idles > 10) && (tp->threads > ts->min_thread)){
                share->tid = 0;
                __sync_sub_and_fetch(&tp->threads, 1);
                ZCWARN("thread exit, idles:%d", idles);
                pthread_exit(NULL);
            }
            continue;
        }

        idles = 0;
        share->start_time = time(NULL);
        share->concur += 1;

        ts->run(task);

        share->concur -= 1;
        share->perform++;
    }

    ZCWARN("thread end!!!");
    return NULL;
}


void  
zc_threadpool_start(zcThreadPool *tp)
{
    if (NULL == tp->setting.run) {
        ZCERROR("setting function must not NULL\n");
        return;
    }
    ZCINFO("create setting count:%d\n", tp->setting.min_thread);
    zc_threadpool_child(tp, &tp->setting, tp->setting.min_thread-1);
}

int 
zc_threadpool_put(zcThreadPool *tp, zcTask *task, int timeout)
{
    if (timeout <= 0) {
        return zc_queue_put(tp->queue, task, true, 0);
    }else{
        return zc_queue_put(tp->queue, task, true, timeout);
    }
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



