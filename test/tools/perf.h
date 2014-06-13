#include <stdio.h>
#include <zocle.h>
#include <limits.h>
#include <stdint.h>

#define WAY_THREAD  1
#define WAY_EVENT   2
#define WAY_SINGLE  3

typedef struct _conf
{
    int way;
    int num;
    int longconn;
    int client;
	char input[PATH_MAX];
}Config;


typedef struct _param
{
	int			id;
	int			count;
	int			ret;
	zcCString	*data;
	uint64_t	ctime;
	uint64_t	stime;
	uint64_t	etime;
	Config		conf;
}TestParam;


