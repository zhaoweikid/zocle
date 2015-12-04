#ifndef ZOCLE_SERVER_THREADPOOL_H
#define ZOCLE_SERVER_THREADPOOL_H

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <zocle/base/defines.h>
#include <zocle/ds/queue.h>
#include <zocle/server/task.h>

#define ZC_THREADPOOL_MAX  512

#define ZC_THREADPOOL_STOP	0
#define ZC_THREADPOOL_RUN	1

typedef struct zc_threadshare_t
{
    pthread_t tid;     //!< 线程id
    time_t    create_time; //!< 线程创建时间
    time_t    start_time;  //!< 线程最后一次开始执行的时间
    void     *user_data; //!< 线程的用户私有数据
    uint32_t  perform; //!< 已经处理的任务数量
    uint16_t  concur;  //!< 当前并发数 
    char      status;  //!< 线程状态
}zcThreadInfo;

typedef struct zc_threadsetting_t
{
    uint16_t        max_thread;  //!< 线程最大数量
    uint16_t        min_thread;  //!< 线程最小数量
    uint16_t        idle_thread; //!< 线程允许的空闲进程数
    uint16_t        inc_step;//!< 线程增长速度
    zcFuncRun       run;     //!< 线程的执行函数
    char            status;
}zcThreadSetting;

struct zc_threadpool_t;
typedef struct zc_threadpool_t zcThreadPool;

struct zc_threadpool_t
{
	zcThreadSetting setting;

    volatile int    threads;     //!< 线程的当前线程数
    zcThreadInfo    *share;   //!< 线程信息
    zcQueue         *queue;
	void	        *user_data;
    pthread_mutex_t lock;
};

zcThreadPool* zc_threadpool_new(int qsize, int maxthread, int minthread, int idle, zcFuncRun run);
zcThreadPool* zc_threadpool_new_setting(int qsize, zcThreadSetting *consumer);
void  zc_threadpool_delete(void*);
void  zc_threadpool_start(zcThreadPool*);

int   zc_threadpool_put(zcThreadPool *, zcTask *task, int);

#endif
