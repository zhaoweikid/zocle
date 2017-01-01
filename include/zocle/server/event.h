#ifndef ZOCLE_EVENT
#define ZOCLE_EVENT

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#define ZC_LOOP_RUN		1
#define ZC_LOOP_STOP	2

struct zc_loop_t;
typedef struct zc_loop_t zcLoop;

typedef int (*zcFuncEvent)(zcLoop*, int fd, uint32_t ev, void *data);

typedef struct zc_loop_event_t
{
	zcFuncEvent func;	
	void		*data;
}zcLoopEvent;

struct zc_loop_t
{
	int			efd;
	uint8_t		state;
	zcLoopEvent	evt[65536];
	int			timeout;
};

zcLoop* zc_loop_create();
void	zc_loop_delete(void*);
int		zc_loop_start(zcLoop *);
int		zc_loop_add_ev(zcLoop *, int fd, uint32_t evts, zcFuncEvent f, void*);
int		zc_loop_mod_ev(zcLoop *, int fd, uint32_t evts, zcFuncEvent f, void*);
int		zc_loop_del_ev(zcLoop *, int fd, uint32_t evts, zcFuncEvent f, void*);


#endif
