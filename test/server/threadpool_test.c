#include <zocle/zocle.h>

void my_producer(zcThreadPool *th)
{
    long long i = 0; 
    while (1) {
        zc_queue_put(th->queue, (void*)i, 1, 0);
        usleep(0.5); 
        i++;
    }
}

void* my_consumer(void *x)
{
    zcTask *t = (zcTask*)x;
    
    ZCINFO("task:%s", (char*)t->data);
    sleep(1);

    return NULL;
}


int test()
{
    zcThreadPool *th = zc_threadpool_new(10, 10, 1, 1, my_consumer);
    if (th == NULL) {
        ZCERROR("threadpool new error");
        return -1;
    }

    zc_threadpool_start(th);

    my_producer(th);

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
