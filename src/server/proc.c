#ifndef _WIN32
#include <zocle/server/proc.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <zocle/system/util.h>
#include <errno.h>

zcProc  *_zc_proc;

static void
child_print(int signal)
{
    ZCERROR("<<<<<< proc %d signal %d !!! >>>>>>", getpid(), signal);
    //abort();
    exit(0);
}

static void
child_wait(int signal)
{
    pid_t       child;
    zcProcShare *base, *share;
    
    ZCINFO("child exit.\n");
    base = _zc_proc->share;
    if (NULL == base) {
        ZCERROR("NULL == base!\n");
        return;
    }   
    ZCINFO("wait pid ...");
    while ((child = waitpid(-1, NULL, WNOHANG)) > 0) {
        int i;
        for (i = 1; i < ZC_PROC_MAX; i++) {
            ZCINFO("find proc at %d", i);
            share = base + i;
            if (share->pid == child) {
                ZCINFO("ok, found");
                share->pid *= -1; 
                break;
            }   
        }   
    }
}

static void 
zc_proc_reset(zcProc *proc)
{
    proc->req    = 0;
    proc->concur = 0;
    proc->status = ZC_PROC_INIT;
}


zcProc*	
zc_proc_new(uint32_t maxp, uint32_t minp, uint32_t idlep, 
            uint32_t maxreq, uint64_t maxm)
{
    if (minp > maxp || maxp > ZC_PROC_MAX || minp > ZC_PROC_MAX || idlep >= maxp) {
        ZCWARN("max_proc/min_proc/idlep error!");
        return NULL;
    }
    zcProc *proc = (zcProc*)zc_malloc(sizeof(zcProc));
    memset(proc, 0, sizeof(zcProc));
    
    zc_signal_set(SIGTERM, child_print);
    zc_signal_set(SIGQUIT, child_print);
    zc_signal_set(SIGCHLD, child_wait);

    proc->max_proc   = maxp;
    proc->min_proc   = minp;
    proc->idle_proc  = idlep;
    proc->max_req    = maxreq;
    //proc->max_concur = 0;
    proc->max_mem    = maxm;
    proc->check_interval = 3;
    proc->id = -1;

    int shmid, ret;
    shmid = shmget(IPC_PRIVATE, sizeof(zcProcShare)*ZC_PROC_MAX, 
                    IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);    
    if (shmid < 0) {
        ZCERROR("set share memory error!\n");
        ZC_EXIT();
        return NULL;
    }
    proc->share = shmat(shmid, NULL, 0);
    if (NULL == proc->share) {
        ZCERROR("shmat error!\n");
        ZC_EXIT();
        return NULL;
    }
    memset(proc->share, 0, sizeof(zcProcShare)*ZC_PROC_MAX);    
    ret = shmctl(shmid, IPC_RMID, 0);
    if (ret < 0) {
        ZCERROR("shmctl error:%s\n", strerror(errno));
        ZC_EXIT();
        return NULL;
    }
    
    char path[64];
    snprintf(path, sizeof(path), "/zc_proc_%d", getpid());
    if (zc_plock_init(&proc->locker, path) != ZC_OK) {
        ZCERROR("zc_plock_new error!\n");
        ZC_EXIT();
        return NULL;
    }
    
    _zc_proc = proc;

    return proc;
}

void	
zc_proc_delete(void *x)
{
    zcProc *proc = (zcProc*)x;
    zc_plock_destroy(&proc->locker);
    zc_free(x);
}

static int
zc_proc_child_init(zcProc *proc)
{
    zcProcShare   *base;
    zcProcShare   *share;

    base = proc->share;
    zc_proc_reset(proc);
    zc_plock_lock(&proc->locker);
    int i;
    for (i = 1; i < ZC_PROC_MAX; i++) {
        share = base + i;
        if (share->pid <= 0) {
            break;
        }
    }
    if (i == ZC_PROC_MAX) {
        ZCWARN("not found new pos in share");
        zc_plock_unlock(&proc->locker);
        return ZC_FAILED;
    }
    share->pid = getpid();
    share->status = 0;
    share->ctime = time(NULL);
    share->load = 0;

    ZCINFO("proc id: %d\n", i);
    proc->id = i;
    zc_plock_unlock(&proc->locker);

    return ZC_OK;
}

static int
zc_proc_child_runloop(zcProc *proc)
{
    ZCINFO("runloop proc:%d", proc->id);
    zcProcShare  *base  = (zcProcShare*)proc->share;
    zcProcShare  *share = base + proc->id;
    
    share->status = ZC_PROC_RUNNING;
    
    while (share->status == ZC_PROC_RUNNING) {
        proc->concur++;
        //ZCINFO("run child");
        if (proc->run == NULL) {
            ZCERROR("proc->run must not NULL");
            sleep(1);
            break;
        }
        int ret = proc->run(proc);
        proc->req++;
        if (ret < 0) {
            ZCINFO("run error:%d", ret);
            break;
        }
        //ZCINFO("run child end");
        proc->concur--;
    }
    share->status = ZC_PROC_STOPPED;
    //ZCINFO("runloop end");
    return ZC_OK;
}

static int
zc_proc_child_create(zcProc *proc, int num)
{
    pid_t   newpid;
    int i;
    for (i=0; i<num; i++) {
        newpid = fork(); 
        if (newpid == 0) { // chlid
            ZCINFO("init new child ...");
            zc_proc_child_init(proc);
            if (proc->create) {
                ZCINFO("run create");
                proc->create(proc);
            }

            zc_proc_child_runloop(proc);

            if (proc->destroy) {
                ZCINFO("run destroy");
                proc->destroy(proc);
            }
            exit(0);
        }else if (newpid > 0) { // parent
            ZCNOTE("create child %d\n", newpid);
        }else{ // error
            ZCERROR("fork error!");
            return ZC_ERR;
        }
    }
    
    return ZC_OK;
}

static int
zc_proc_child_reduce(zcProc *proc, int num)
{
    ZCINFO("reduce %d", num);
    if (num <= 0)
        return ZC_OK;

    return ZC_OK;
}

int	
zc_proc_run(zcProc *proc)
{
    zcProcShare  *base = (zcProcShare*)proc->share;

    base->pid   = getpid();
    base->ctime = time(NULL);
    base->status= ZC_PROC_RUNNING;
    base->load  = 0;

    zc_proc_child_create(proc, proc->min_proc);
    //sleep(3);

    while (1) {
        sleep(proc->check_interval);
        //ZCINFO("proc check: %p", proc->check);
        if (proc->check) {
            proc->check(proc);
        }else{
            zc_proc_check(proc);
        }
        //ZCINFO("check interval:%d", proc->check_interval);
    }
    return ZC_OK;
}

int	
zc_proc_check(zcProc *proc)
{
    ZCINFO("proc check ...\n");
    zcProcShare  *base = (zcProcShare*)proc->share;
    zcProcShare  *item;
    int i;
    int procs = 0;
    int loads = 0;
    for (i=1; i<ZC_PROC_MAX; i++) {
        item = base + i; 
        if (item->pid > 0) {
            ZCINFO("found proc %d", item->pid);
            procs++;
            loads += item->load;
        }
    }
    int loadavg = 0;
    if (loads > 0 && procs > 0) {
        loadavg = (int)((double)loads / procs);
    }
    ZCINFO("procs:%d, loads:%d, loadavg:%d", procs, loads, loadavg);
    
    if (procs < proc->min_proc) {
        ZCINFO("smaller than min_proc:%d, create %d", proc->min_proc, proc->min_proc-procs);
        zc_proc_child_create(proc, proc->min_proc-procs);
        return ZC_OK;
    }

    if (loadavg >= 80) {
        zc_proc_child_create(proc, 12-(100-loadavg)/2);
    }else if (loadavg <= 50) {
        if (procs > proc->min_proc) {
            zc_proc_child_reduce(proc, (1-(double)loadavg/50)*procs);
        }
    }
    
    return ZC_OK;
}

#endif
