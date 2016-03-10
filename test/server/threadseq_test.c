#include <zocle/zocle.h>

int i;

void* my_producer(void *x)
{
    char *a = zc_malloc(128);
    snprintf(a, 128, "producer %d", i);
    ZCINFO("%s", a);
    sleep(1); 
    i++;
    return a;
}

void* my_consumer(void *x)
{
    char *a = (char*)x;

    ZCINFO("consumer %s", a);
    //sleep(1);

    zc_free(a);

    return NULL;
}


int test()
{
    zcThreadSeq *t = zc_threadseq_new(10);
    if (t == NULL) {
        ZCERROR("threadseq new error");
        return -1;
    }
    t->consume = my_consumer;
    t->produce = my_producer;

    zc_threadseq_start(t);

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
