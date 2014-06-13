#include <zocle/zocle.h>

long long i;

void* my_producer(void *x)
{
    zcThreadQParam *param = (zcThreadQParam*)x;
    ZCINFO("producer:%lld", i);
    param->task = (void*)i;
    sleep(1); 
    i++;
    return param;
}

void* my_consumer(void *x)
{
    zcThreadQParam *param = (zcThreadQParam*)x;
    ZCINFO("consumer:%lld", (long long)param->task);
    //sleep(1);

    return NULL;
}


int test()
{
    zcThreadQueue *t = zc_threadqueue_new(10);
    if (t == NULL) {
        ZCERROR("threadqueue new error");
        return -1;
    }
    t->consume = my_consumer;
    t->produce = my_producer;

    zc_threadqueue_start(t);

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
