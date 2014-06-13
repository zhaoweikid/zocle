#ifndef ZOCLE_SYSTEM_PLOCK_H
#define ZOCLE_SYSTEM_PLOCK_H

#include <stdio.h>
#include <zocle/mem/alloc.h>
#include <zocle/log/logfile.h>
#include <zocle/base/defines.h>

#ifdef _WIN32
#include <windows.h>

struct zc_plock_t
{
    HANDLE       mutex;
};


#else
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

struct zc_plock_t
{
    sem_t       *mutex;
};


#endif

struct zc_plock_t;
typedef struct zc_plock_t    zcPLock;

zcPLock*    zc_plock_new(char *path);
int         zc_plock_new2(zcPLock**, char *path);
void        zc_plock_delete(void *); 

int         zc_plock_init(zcPLock *, char *path);
int        zc_plock_destroy(void *); 

int         zc_plock_lock(zcPLock*);
int         zc_plock_trylock(zcPLock*);
int         zc_plock_timedlock(zcPLock*, uint32_t msec);
int         zc_plock_unlock(zcPLock*);


#endif
