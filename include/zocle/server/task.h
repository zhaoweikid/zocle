#ifndef ZOCLE_SERVER_TASK_H
#define ZOCLE_SERVER_TASK_H

#include <stdio.h>
#include <stdint.h>

typedef struct zc_task_t
{
	int 	readfd;
	int 	writefd;
	int64_t	ctime;
	void	*data;
	void	*result;
}zcTask;

zcTask* zc_task_new();
void	zc_task_delete(void*);
int		zc_task_put(zcTask*, void*);
int		zc_task_wait(zcTask*, int timeout);
int		zc_task_result(zcTask*, void *result);


#endif
