#ifndef ZOCLE_UTILS_POOL_H
#define ZOCLE_UTILS_POOL_H

#include <stdio.h>
#include <pthread.h>
#include <zocle/ds/list.h>
#include <zocle/base/defines.h>

typedef struct zc_poolitem_t
{
	void	*data;
	uint32_t uptime;
}zcPoolItem;

typedef struct zc_pool_t
{
	zcList		*idle;
	zcList		*used;

	int			max_all;  // 最大项目数
	int			max_idle; // 最大空闲项目
	int			timeout;

	zcFuncNew	newfunc;	
	zcFuncDel	delfunc;
	void		*userdata;

	pthread_mutex_t mutex;
	pthread_cond_t	cond;
}zcPool;

zcPool*	zc_pool_new(int maxidle, int maxitem, int timeout, zcFuncNew newf, zcFuncDel delf);
void	zc_pool_delete(void*);
void*	zc_pool_get(zcPool *, int timeout, void* defv);
void	zc_pool_put(zcPool *, void*);


#endif
