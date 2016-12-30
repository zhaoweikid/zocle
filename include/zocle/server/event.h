#ifndef ZOCLE_EVENT
#define ZOCLE_EVENT

#include <stdio.h>
#include <unistd.h>

#define ZC_LOOP_RUN		1
#define ZC_LOOP_STOP	2



typedef int (*zcFuncEvent)(zcLoop*, int fd, uint32_t ev, void *data);

typedef struct zc_event_call_t
{
	zcFuncEvent func;	
	void		*data;
}zcEventCall;

typedef struct zc_loop_t
{
	int		efd;
	uint8_t	state;
	zcEvent	evt[65536];
}zcLoop;

zcLoop* zc_loop_create();
void	zc_loop_delete(void*);
int		zc_loop_start(zcLoop *);
void	zc_loop_add_ev(zcLoop *, int fd, int ev, zcFuncEvent e, void*);
void	zc_loop_mod_ev(zcLoop *, int fd, int ev, zcFuncEvent e, void*);
void	zc_loop_del_ev(zcLoop *, int fd, int ev, zcFuncEvent e, void*);


#endif
