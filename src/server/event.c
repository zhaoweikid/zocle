#include <zocle/server/event.h>

zcLoop* 
zc_loop_create()
{
    zcLoop *loop = zc_calloct(zcLoop);
    if (NULL == loop)
        return NULL;
    loop->efd = epoll_create(1);
    if (loop->efd < 0) {
        ZCERROR("epoll create error:%d", loop->efd);
        zc_free(loop);
        return NULL;
    }
    loop->state = ZC_LOOP_RUN;
    return loop;
}

void	
zc_loop_delete(void* x)
{
    zcLoop *loop = (zcLoop*)x;
    close(x->loop->efd); 
    zc_free(x);
}

#define MAX_EVENTS 10000
int		
zc_loop_start(zcLoop *loop)
{
    int n, i;
    uint32_t eval;
    int fd;
    struct epoll_event ev, events[MAX_EVENTS];
    while (loop->state == ZC_LOOP_RUN) {
        n = epoll_wait(loop->efd, events, MAX_EVENTS, loop->timeout);
        if (n == 0) { // no event
            continue;
        }
        if (n < 0) { // error
            ZCWARN("epoll wait error: %d", n);
            return ZC_ERR;
        }

        for (i=0; i<n; i++) {
            eval = events[i].events;
            fd = events[i].data.fd;
            ecall = loop->evt[fd];
            if (NULL == ecall) {
                ZCWARN("not found event call func: %d", fd);
                continue;
            }
            if (eval | EPOLLIN ) {
                ZCDEBUG("epollin %d", fd);
                ecall->func(loop, fd, EPOLLIN, ecall->data);
            }
            if (eval | EPOLLOUT) {
                ZCDEBUG("epollout %d", fd);
                ecall->func(loop, fd, EPOLLOUT, ecall->data);
            }
            if (eval | EPOLLERR) {
                ZCDEBUG("epollerr %d", fd);
                //ecall->func(loop, fd, EPOLLIN, ecall->data);
            }
        }

    }
}


void	
zc_loop_add_ev(zcLoop *loop, int fd, uint32_t evts, zcFuncEvent f, void *data)
{

    zcEventCall *call = zc_calloct(zcEventCall);
    call->func = f;
    call->data = data;

    struct epoll_event evt;
    evt.events = evts;
    evt.data.fd = fd;
    evt.data.ptr = call;
    
    int ret = epoll_ctl(loop->efd, EPOLL_CTL_ADD, fd, &evt);
    if (ret < 0) {
        ZCWARN("epoll_ctl error, fd:%d, evts:%u, ret:%d, errno:%d", fd, evts, ret, errno);
    }
}

void	
zc_loop_mod_ev(zcLoop *, int fd, int ev, zcFuncEvent f, void *data)
{
    
}


void	
zc_loop_del_ev(zcLoop *, int fd, int ev, zcFuncEvent f, void *data)
{
}



