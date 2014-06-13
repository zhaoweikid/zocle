#ifndef _WIN32
#include <zocle/system/plock.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <errno.h>

#define SEM_MUTEX_FILE  "/zocle_proc_mutex"

zcPLock*
zc_plock_new(char *path)
{
    zcPLock  *k;
    int     ret;

    ret = zc_plock_new2(&k, path);
    if (ret != ZC_OK)
        return NULL;
    return k;
}

int
zc_plock_new2(zcPLock** zplk, char *path)
{
    zcPLock     *tlk;

    *zplk = NULL;

    tlk = malloc(sizeof(zcPLock));
    
    if (NULL == path)
        path = SEM_MUTEX_FILE;

    if (zc_plock_init(tlk, path) != ZC_OK) {
        zc_free(tlk);
        return ZC_ERR;
    } 

    *zplk = tlk;
    return ZC_OK;
}

void
zc_plock_delete(void *t)
{
    zc_plock_destroy(t);
    zcPLock  *k = t;
    zc_free(k);
}

int
zc_plock_init(zcPLock *k, char *path)
{
    if (NULL == path)
        path = SEM_MUTEX_FILE;
    memset(k, 0, sizeof(zcPLock));
    k->mutex = sem_open (path, O_RDWR|O_CREAT|O_EXCL, 0644, 1);
    if (SEM_FAILED == k->mutex) {
        ZCERROR("sem_open error! %s\n", strerror(errno));
        return ZC_ERR;
    }
    sem_unlink (path);

    return ZC_OK;
}

int
zc_plock_destroy(void *k)
{
    zcPLock *pk = (zcPLock*)k;
    sem_close(pk->mutex);
    return ZC_OK;
}


int
zc_plock_lock(zcPLock *k)
{
    int ret;
    do{
        ret = sem_wait(k->mutex);
    } while (ret != 0 && errno == EINTR);
    return ret;
}

int
zc_plock_trylock(zcPLock *k)
{
    int ret;
    do{
        ret = sem_trywait(k->mutex);
    } while (ret != 0 && errno == EINTR);
    return ret;
}


int
zc_plock_timedlock(zcPLock *k, uint32_t msec)
{
#ifdef __APPLE__
    return 0;
#else
    int ret;
    struct timespec ts;

    ts.tv_sec = time(NULL) + msec/1000000;
    ts.tv_nsec = msec%1000000;
    do{
        ret = sem_timedwait(k->mutex, &ts);
    } while (ret != 0 && errno == EINTR);
    return ret;
#endif
}

int
zc_plock_unlock(zcPLock *k)
{
    return sem_post(k->mutex);
}

#endif
