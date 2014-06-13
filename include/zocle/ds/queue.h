#ifndef ZOCLE_DS_QUEUE_H
#define	ZOCLE_DS_QUEUE_H

#include <pthread.h>
#include <zocle/base/defines.h>

typedef struct zc_queue_t
{
    int size;
    int length;
    int head;
    int tail;

    void **data;
    zcFuncDel del;
    pthread_mutex_t _lock;
    pthread_cond_t  _full;
    pthread_cond_t  _empty;
}zcQueue;


ZOCLE_API zcQueue*  zc_queue_new (int size);
ZOCLE_API int       zc_queue_new_co (zcQueue**, int size);
ZOCLE_API void      zc_queue_delete (void*);
ZOCLE_API void      zc_queue_destroy (void*);
ZOCLE_API void      zc_queue_clear (zcQueue*);
ZOCLE_API int       zc_queue_put (zcQueue*, void*, bool, int);
ZOCLE_API void*     zc_queue_get (zcQueue*, bool, int, void*);
ZOCLE_API int       zc_queue_isempty (zcQueue*);
ZOCLE_API int       zc_queue_isfull (zcQueue*);
ZOCLE_API int       zc_queue_length (zcQueue*);
ZOCLE_API int       zc_queue_resize (zcQueue*, int);
ZOCLE_API void      zc_queue_print (zcQueue*);


#endif
