#ifndef ZOCLE_LOG_LOGFILE_H
#define ZOCLE_LOG_LOGFILE_H

#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

#define ZC_LOG_NOLOG	0
#define ZC_LOG_FATAL    1
#define ZC_LOG_ERROR    2
#define ZC_LOG_WARN     3
#define ZC_LOG_NOTICE   4
#define ZC_LOG_INFO     5
#define ZC_LOG_ALL      6

#define ZC_LOG_ROTATE_NO		0
#define ZC_LOG_ROTATE_SIZE		1
#define ZC_LOG_ROTATE_TIME		2
#define ZC_LOG_ROTATE_TIMEAT	3
#define ZC_LOG_ROTATE_REOPEN	4
#define ZC_LOG_ROTATE_WATCH	    5

#define ZC_LOG_SUFFIX_NUM	1
#define ZC_LOG_SUFFIX_TIME	2
#define ZC_LOG_SUFFIX_PID	3

#define ZC_LOG_FILE		1
#define ZC_LOG_SYSLOG	2
#define ZC_LOG_TCP		3
#define ZC_LOG_UDP		4

typedef struct zc_logitem_t
{
	char filename[PATH_MAX];
	char protocol;
	char host[64];
	int  port;
	int  fd;
	int  level;
	int  dup;
	volatile uint32_t last_rotate_time;
    volatile uint32_t last_check_time; // last check time
	struct sockaddr_in sin;
}zcLogItem;

zcLogItem* zc_logitem_new(const char *filename, int level, int dup);
void	   zc_logitem_delete(void *);

typedef struct zc_logfile_t
{
	zcLogItem items[ZC_LOG_ALL+1];
    //int      log_type; // file/syslog/tcp/udp
	int		 rotate_type;
	int		 suffix; // end of file
    int      loglevel;
	int		 logwhole; // write whole log
	int		 count; // number of log file
	int		 timeat[3]; //hour,minute,second
    char     logprefix[128];
    uint32_t maxsize;
	uint32_t maxtime;
	uint32_t check_interval;
    pthread_mutex_t lock;
}zcLog;

extern zcLog    *_zc_log;

zcLog*  zc_log_new(const char *filename, int loglevel);
void    zc_log_delete(void *log);
int		zc_log_file(zcLog *log, const char *filename, int loglevel, int dup);
int     zc_log_rotate_size(zcLog *log, int logsize, int logcount);
int     zc_log_rotate_time(zcLog *log, int logtime, int logcount);
int     zc_log_rotate_timeat(zcLog *log, int day, int hour, int min, int logcount);
void    zc_log_rotate_no(zcLog *log);
void	zc_log_whole(zcLog *log, int flag);
void    zc_log_set_prefix(zcLog *log, char *prefix);
int		zc_log_init(zcLog *log, const char *filename, int loglevel);
void    zc_log_destroy(void *log);
void	zc_log_flush(zcLog *log);
int     zc_log_write(zcLog *log, int level, const char *file, int line, const char *format, ...);
int		zc_log_check_rotate(zcLog *log, int loglevel);

#define zc_log_whole_true() zc_log_whole(_zc_log, ZC_TRUE);
#define zc_log_delete_null(x) do{zc_log_delete(x,NULL);x=NULL;}while(0)

#define ZCFATAL(format,args...) \
    do{\
        if (NULL == _zc_log) {\
            fprintf(stderr, format, ##args);\
            ZOCLE_ABORT();\
        }else{\
            if (_zc_log->loglevel >= ZC_LOG_FATAL) {\
                zc_log_write(_zc_log, ZC_LOG_FATAL, __FILE__, __LINE__, format, ##args);\
                ZOCLE_ABORT();\
            }\
        }\
    }while(0)

#define ZCERROR(format,args...) \
    do{\
        if (NULL == _zc_log) {\
            fprintf(stderr, format, ##args);\
        }else{\
            if (_zc_log->loglevel >= ZC_LOG_ERROR) {\
                zc_log_write(_zc_log, ZC_LOG_ERROR, __FILE__, __LINE__, format, ##args);\
            }\
        }\
    }while(0)

#define ZCWARN(format,args...) \
    do{\
        if (NULL == _zc_log) {\
            fprintf(stderr, format, ##args);\
        }else{\
            if (_zc_log->loglevel >= ZC_LOG_WARN) {\
                zc_log_write(_zc_log, ZC_LOG_WARN, __FILE__, __LINE__, format, ##args);\
            }\
        }\
    }while(0)


#define ZCNOTICE(format,args...) \
    do{\
        if (NULL == _zc_log) {\
            fprintf(stderr, format, ##args);\
        }else{\
            if (_zc_log->loglevel >= ZC_LOG_NOTICE) {\
                zc_log_write(_zc_log, ZC_LOG_NOTICE, __FILE__, __LINE__, format, ##args);\
            }\
        }\
    }while(0)

#define ZCNOTE	ZCNOTICE

#define ZCINFO(format,args...) \
    do{\
        if (NULL == _zc_log) {\
            fprintf(stderr, format, ##args);\
        }else{\
            if (_zc_log->loglevel >= ZC_LOG_INFO) {\
                zc_log_write(_zc_log, ZC_LOG_INFO, __FILE__, __LINE__, format, ##args);\
            }\
        }\
    }while(0)

#define ZCPRINT(level,file,line,format,args...) \
    do{\
        if (NULL == _zc_log) {\
            fprintf(stderr, format, ##args);\
        }else{\
            zc_log_write(_zc_log, level, file, line, format, ##args);\
        }\
    }while(0)


#endif
