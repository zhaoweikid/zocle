#ifndef _WIN32
#ifndef ZOCLE_SERVER_PROC_H
#define ZOCLE_SERVER_PROC_H

#include <stdint.h>
#include <unistd.h>
#include <zocle/system/plock.h>

#define ZC_PROC_INIT		1
#define ZC_PROC_RUNNING		2
#define ZC_PROC_STOPPING	3
#define	ZC_PROC_STOPPED		4

#define ZC_PROC_MAX			4096

struct zc_proc_t;
typedef struct zc_proc_t zcProc;

typedef int (*zcProcCreateFunc)(zcProc *p);
typedef int (*zcProcDestroyFunc)(zcProc *p);
typedef int (*zcProcRunFunc)(zcProc *p);
typedef int (*zcProcCheckFunc)(zcProc *p);

typedef struct zc_proc_share_t
{
	pid_t		pid;
	uint8_t		status;
	uint32_t	ctime;
	uint32_t	load;
}zcProcShare;

/*typedef struct __zocle_proc_info
{
	uint32_t	req; // 已处理请求数
	uint32_t	concur; // 当前并发
}zcProcInfo;*/

typedef struct zc_proc_stat_t
{
	uint32_t	req1;
	uint32_t	req5;
	uint32_t	req15;
}zcProcStat;

struct zc_proc_t
{
	int id;
	// param
	uint32_t max_proc;
	uint32_t min_proc;
	uint32_t idle_proc;
	uint32_t max_req;
	uint32_t max_concur;
	uint64_t max_mem;
	int check_interval;

	int	status;				
	zcProcShare *share;
	//zcProcInfo	*info;

	zcPLock		locker;
	void		*userdata;
	// callback
	zcProcCreateFunc	create;
	zcProcDestroyFunc	destroy;
	zcProcRunFunc		run;
	zcProcCheckFunc		check;
	// for state
	uint32_t	req;
	uint32_t	concur;
};

zcProc*	zc_proc_new(uint32_t maxp, uint32_t minp, uint32_t idelp, 
					uint32_t maxreq, uint64_t maxm);
void	zc_proc_delete(void*);
int		zc_proc_run(zcProc*);
int		zc_proc_check(zcProc*);

extern zcProc* g_proc;

#endif
#endif

