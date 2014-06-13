#include <zocle/utils/pool.h>
#include <zocle/mem/alloc.h>
#include <zocle/log/logfile.h>
#include <zocle/ds/listhead.h>
#include <errno.h>
#include <stdlib.h>

static int zc_poolitem_cmp(void *left, void *right, int len)
{
    zcPoolItem *one = (zcPoolItem*)left;
    return one->data - right;
}


zcPool*
zc_pool_new(int maxidle, int maxitem, int timeout, zcFuncNew newf, zcFuncDel delf)
{
    zcPool *p = zc_calloct(zcPool);

    p->idle = zc_list_new();
    p->idle->cmp = zc_poolitem_cmp;
    p->used = zc_list_new();
    p->used->cmp = zc_poolitem_cmp;

    p->max_idle = maxidle;
    p->max_all  = maxitem;
    p->timeout  = timeout;
    p->new = newf;
    p->del = delf;

    int ret;
    ret = pthread_mutex_init(&p->mutex, NULL);
    if (ret != 0) {
        goto over;
    }
    ret = pthread_cond_init(&p->cond, NULL);
    if (ret != 0) {
        pthread_mutex_destroy(&p->mutex);
        goto over;
    }

    return p;

over:
    zc_list_delete(p->idle);
    zc_list_delete(p->used);
    zc_free(p); 

    return NULL;
}

void
zc_pool_delete(void *x)
{
    zcPool *p = (zcPool*)x;
    
    zcPoolItem *item;
    zcListNode *node;
    zc_list_foreach(p->idle, node) {
        item = (zcPoolItem*)node->data;
        if (p->del) {
            p->del(item->data);
        }
        zc_free(item);
    }
    zc_list_foreach(p->used, node) {
        item = (zcPoolItem*)node->data;
        if (p->del) {
            p->del(item->data);
        }
        zc_free(item);
    }

    zc_list_delete(p->idle);
    zc_list_delete(p->used);
    
    pthread_mutex_destroy(&p->mutex);
    pthread_cond_destroy(&p->cond);

    zc_free(p); 
}

static int 
zc_pool_newitem(zcPool *p, int n)
{
    int i;
    for (i=0; i<n; i++) {
        void *x = p->new(p->userdata);
        zcPoolItem *item = zc_calloct(zcPoolItem);
        item->data = x;
        item->uptime = time(NULL);
        zc_list_prepend(p->idle, item);
    }
    return ZC_OK;
}

static int
zc_pool_clear_timeout(zcPool *p)
{
    ZCINFO("clear timeout ...\n");
    uint32_t now = time(NULL);
    pthread_mutex_lock(&p->mutex);
    if (p->idle->size > p->max_idle) {
        zcPoolItem *item = NULL;
        zcListNode *node, *tmp;
        zc_list_foreach_safe(p->idle, node, tmp) {
            item = (zcPoolItem*)node->data; 
            if (now - item->uptime > p->timeout) {
                ZCINFO("remove %p\n", item->data);
                zc_list_delete_node(p->idle, node);
            }
        }
    }
    pthread_mutex_unlock(&p->mutex);

    return ZC_OK;
}

void* 
zc_pool_get(zcPool *p, int timeout, void *defv)
{
    int ret;

    pthread_mutex_lock(&p->mutex);
    
    int c = p->idle->size + p->used->size;

    if (p->idle->size == 0 && p->max_all > 0 && c < p->max_all) {
        zc_pool_newitem(p, 1);

        zcPoolItem *one = zc_list_pop(p->idle, 0, NULL);
        one->uptime = time(NULL);
        zc_list_prepend(p->used, one);
        pthread_mutex_unlock(&p->mutex);

        return one->data;
    }
    
    while (p->idle->size == 0) {
        struct timespec ts;
        //clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec  = time(NULL) + timeout/1000;
        ts.tv_nsec = timeout%1000 * 1000;

        if (timeout > 0) {
            ret = pthread_cond_timedwait(&p->cond, &p->mutex, &ts);
            if (EINTR == ret)
                continue;
            if (ETIMEDOUT == ret) {
                pthread_mutex_unlock(&p->mutex);
                return defv;
            }
            continue;
        }else{
            ret = pthread_cond_wait(&p->cond, &p->mutex);
            if (EINTR == ret)
                continue;
            return defv;
        }
    }

    zcPoolItem *one = (zcPoolItem*)zc_list_pop(p->idle, 0, NULL);
    zc_list_prepend(p->idle, one);
    one->uptime = time(NULL);
    pthread_mutex_unlock(&p->mutex);

    if (random() % 100 > 80) {
        zc_pool_clear_timeout(p);
    }

    return one->data;
}


void
zc_pool_put(zcPool *p, void *x)
{
    pthread_mutex_lock(&p->mutex);
   
    int i;
    struct zc_listhead_t *node = zc_list_find_data(p->used, x, &i);
    if (node) {
        zc_listhead_del_entry(node);
        zc_list_prepend(p->idle, node);
        //zcListNode *listnode = zc_listhead_entry(node, zcListNode, head);
    }
    pthread_cond_signal(&p->cond);
    pthread_mutex_unlock(&p->mutex);
}


