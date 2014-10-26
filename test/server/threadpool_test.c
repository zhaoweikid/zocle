#include <zocle/zocle.h>

void* my_producer(void *x)
{
    zcThreadParam *param = (zcThreadParam*)x;
    long long i = 0; 
    while (1) {
        zc_queue_put(param->threadpool->queue, (void*)i, 1, 0);
        usleep(0.5); 
        i++;
    }
    return NULL;
}

void* my_consumer(void *x)
{
    zcThreadParam *param = (zcThreadParam*)x;
    
    ZCINFO("consumer:%lld, queue:%d", (long long)param->task, param->threadpool->queue->size);
    sleep(1);

    return NULL;
}


int test()
{
    zcThreadPool *th = zc_threadpool_new(10, 1, 10, 1, 1);
    if (th == NULL) {
        ZCERROR("threadpool new error");
        return -1;
    }
    th->consumer.run = my_consumer;
    th->producer.run = my_producer;

    zc_threadpool_start(th);

    ZCINFO("run end");

    return 0;
}


int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);
    
    test();

    return 0;
}
