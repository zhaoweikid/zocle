#include <zocle/server/task.h>
#include <errno.h>
#include <unistd.h>
#include <zocle/log/logfile.h>
#include <zocle/mem/alloc.h>
#include <zocle/base/defines.h>
#include <zocle/utils/times.h>
#include <poll.h>

zcTask* 
zc_task_new_notify(void *data)
{
    zcTask *t = zc_task_new(data);
    if (NULL == t)
        return NULL;

    int fds[2];
    int ret = pipe(fds); 
    if (ret != 0) {
        ZCWARN("pipe error:%d", ret);
        zc_task_delete(t);
        return NULL;
    }

    t->readfd = fds[0];
    t->writefd = fds[1];

    return t;
}

zcTask* 
zc_task_new(void *data)
{
    zcTask *t = zc_calloct(zcTask);

    t->ctime = zc_timenow();
    t->data = data;
 
    return t;
}

void	
zc_task_delete(void *x)
{
    zcTask *t = (zcTask*)x;
    if (t->readfd > 0) {
        close(t->readfd);
        close(t->writefd);
    }
    zc_free(x);
}

int		
zc_task_set_data(zcTask *t, void *data)
{
    t->ctime = zc_timenow();
    t->data = data;
    return ZC_OK;
}

int		
zc_task_get_result(zcTask *t, int timeout)
{
    if (t->readfd <= 0)
        return ZC_OK;

    struct pollfd pollfd;
     
    while (1) {
        pollfd.fd = t->readfd;
        pollfd.events = POLLIN;

        int n = poll(&pollfd, 1, timeout);
        if (n < 0) { 
            if (errno == EINTR)
                continue;
            return ZC_ERRNO;
        }else if (n == 0) { 
            return ZC_ERR_TIMEOUT;     
        }    
        if (pollfd.revents & POLLIN) {
            break;
        }    
    }
    
    char buf[8];
    int ret = read(t->readfd, buf, 1);
    if (ret != 1)
        return ZC_ERR;

    return ZC_OK;
}

int
zc_task_set_result(zcTask *t, void *result)
{
    t->result = result;
    if (t->readfd > 0) {
        int ret = write(t->writefd, "1", 1);
        if (ret != 1) {
            ZCWARN("write result error! %d", ret);
            return ZC_ERR;
        }
    }
    return ZC_OK;
}



