#include <zocle/ds/queue.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <zocle/log/logfile.h>
#include <zocle/mem/alloc.h>
#include <string.h>
#include <zocle/base/defines.h>

zcQueue*
zc_queue_new (int cap)
{
    zcQueue *q;
    zc_queue_new_co(&q, cap);
    return q;
}

int 
zc_queue_new_co (zcQueue **q, int cap)
{
    zcQueue   *que;
    
    *q = NULL;
    
    que = (zcQueue*)zc_malloc(sizeof(zcQueue));
    memset(que, 0, sizeof(zcQueue));

    que->data = (void**)zc_malloc(cap * sizeof(zcQueue));
    if (NULL == que->data) {   
        zc_free(que);
        return ZC_ERR_NULL;
    }   

    que->cap = cap;

    int ret;
    ret = pthread_mutex_init(&que->_lock, NULL);
    if (ret != 0) {
        ZCERROR("pthread_mutex_init error! %d\n", ret);
        exit(-1);
    }   
    ret = pthread_cond_init(&que->_full, NULL);
    if (ret != 0) {
        ZCERROR("pthread_cond_init error! %d\n", ret);
        exit(-1);
    }   
    ret = pthread_cond_init(&que->_empty, NULL);
    if (ret != 0) {
        ZCERROR("pthread_cond_init error! %d\n", ret);
        exit(-1);
    }
    *q = que;

    return ZC_OK;  
}

void
zc_queue_delete (void *x)
{
    zc_queue_destroy(x);
    zc_free(x);
}

void
zc_queue_destroy (void *x)
{
    zcQueue *q = (zcQueue*)x;
    pthread_mutex_destroy(&q->_lock);
    pthread_cond_destroy(&q->_full);
    pthread_cond_destroy(&q->_empty);
    zc_free(q->data);
}

void
zc_queue_clear (zcQueue *q)
{
    pthread_mutex_lock(&q->_lock);

    int i;
    for (i = 0; i< q->size; i++) {
        // FIXME: not all data will be free
        if (NULL != q->del) {
            q->del(q->data[i]);
        }
    }
    q->size = 0;
    q->head = q->tail = 0;
    pthread_mutex_unlock(&q->_lock);
}

int 
zc_queue_put (zcQueue *q, void *data, bool block, int timeout)
{
    int ret;

    pthread_mutex_lock(&q->_lock);
    while (q->cap == q->size) {
        if (block) {
            if (0 < timeout) {
                /*time_t timenow = time(NULL);
                if (0 >= timetest) {
                    pthread_mutex_unlock(&q->_lock);
                    return ZC_ERR_TIMEOUT;
                }*/
                //ZCINFO("queue put sf, timedwait...\n");
                struct timespec ts;
                ts.tv_sec  = time(NULL)+timeout/1000;
                ts.tv_nsec = timeout%1000 * 1000;

                ret = pthread_cond_timedwait(&q->_full, &q->_lock, &ts);
                if (ETIMEDOUT == ret) {
                    //ZCINFO("pthread_cond_timedwait timeout ...\n");
                    pthread_mutex_unlock(&q->_lock);
                    return ZC_ERR_FULL;
                }else if (EINTR == ret){
                    //ZCINFO("pthread_cond_timedwait interrupt ...\n");
                    //pthread_mutex_unlock(&q->_lock);
                    //return ZC_ERR_FULL;
                    continue;
                }
                //timetest = timeout - (time(NULL) - timenow);
                continue;
            }else{
                pthread_cond_wait(&q->_full, &q->_lock);
                //ZCINFO("pthread_cond_wait over ...\n");
            }
        }else{
            pthread_mutex_unlock(&q->_lock);
            return ZC_ERR_FULL;
        }
    }
   
    q->data[q->tail] = data;
    q->tail++;

    if (q->tail >= q->cap)
        q->tail = 0;
    q->size++;

    pthread_cond_signal(&q->_empty);
    pthread_mutex_unlock(&q->_lock);

    return ZC_OK;
}

void*
zc_queue_get(zcQueue *q, bool block, int timeout, void *defval)
{
    void *retv;
    //int  timetest = timeout;
    int  ret;

    pthread_mutex_lock(&q->_lock);
    while (0 == q->size) {
        if (block) {
            if (0 < timeout) {
                /*time_t  timenow = time(NULL);
                if (0 >= timetest) {
                    pthread_mutex_unlock(&q->_lock);
                    return defval;
                }*/
                //ZCINFO("get run pthread_cond_timedwait \n");
                struct timespec ts;
                ts.tv_sec  = time(NULL)+timeout/1000;
                ts.tv_nsec = timeout%1000 * 1000;

                ret = pthread_cond_timedwait(&q->_empty, &q->_lock, &ts);
                if (ETIMEDOUT == ret) {
                    ZCINFO("get pthread_cond_timedwait timeout ...\n");
                    pthread_mutex_unlock(&q->_lock);
                    return defval;
                }else if (EINTR == ret){
                    ZCINFO("get pthread_cond_timedwait interrupt ...\n");
                    //pthread_mutex_unlock(&q->_lock);
                    //return defval;
                    continue;
                }
                //timetest = timeout - (time(NULL)-timenow)*1000;
                continue;
            }else{
                ZCINFO("get run pthread_cond_wait \n");
                ret = pthread_cond_wait(&q->_empty, &q->_lock);
                ZCINFO("get pthread_cond_wait over %d\n", ret);
            }
        }else{
            pthread_mutex_unlock(&q->_lock);
            ZCINFO("get information no block, return ...\n");
            return defval;
        }
    }

    retv = q->data[q->head];
    q->head++;

    if (q->head >= q->cap)
        q->head = 0;
    q->size--;
    pthread_cond_signal(&q->_full);
    pthread_mutex_unlock(&q->_lock);

    return retv;
}

int 
zc_queue_isempty (zcQueue *q)
{
    if (q->size == 0)
        return ZC_TRUE;
    return ZC_FALSE;
}

int 
zc_queue_isfull (zcQueue *q)
{
    if (q->cap == q->size)
        return ZC_TRUE;
    return ZC_FALSE;
}

int 
zc_queue_size (zcQueue *q)
{
    return q->size;
}

int 
zc_queue_resize (zcQueue *q, int newsize)
{
    void **newdata;
    int  ret = ZC_OK;

    pthread_mutex_lock(&q->_lock);

    if (newsize <= q->size) {
        ret = ZC_ERR_SMALLER;
        goto _resize_over;
    }

    newdata = (void**)zc_malloc(newsize * sizeof(void*));
    if (q->size > 0) {
        if (q->head >= q->tail) {
            int onesz;
            onesz = q->cap - q->head;
            memcpy(newdata, &q->data[q->head], onesz*sizeof(void*));
            memcpy(newdata+onesz, &q->data[0], q->tail*sizeof(void*));
        }else{
            memcpy(newdata, &q->data[q->head], (q->tail-q->head)*sizeof(void*));
        }
        q->head = 0;
        q->tail = q->size;
    }else{
        q->head = q->tail = 0;
    }
    zc_free(q->data);
    q->data = newdata;
    q->cap  = newsize;
_resize_over:
    pthread_mutex_unlock(&q->_lock);

    return ret;
}

void
zc_queue_print (zcQueue *q)
{
    int  i;

    ZCINFO("queue size:%d, cap:%d, head:%d, tail:%d\n", q->size, q->cap, q->head, q->tail);
    if (q->size > 0) {
        if (q->head >= q->tail) {
            for (i = q->head; i < q->cap; i++) {
                ZCINFO("%d | %p | %p\n", i, &q->data[i], q->data[i]);
            }
            for (i = 0; i < q->tail; i++) {
                ZCINFO("%d | %p | %p\n", i, &q->data[i], q->data[i]);
            }
        }else if (q->head < q->tail){
            for (i = q->head; i < q->tail; i++) {
                ZCINFO("%d | %p | %p\n", i, &q->data[i], q->data[i]);
            }
        }
    }
}





