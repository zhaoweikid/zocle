#include <zocle/log/logfile.h>
#include <zocle/mem/alloc.h>
#include <zocle/base/defines.h>
#include <time.h>
#include <sys/timeb.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <zocle/base/compact.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <zocle/utils/datetime.h>
#include <zocle/utils/files.h>

//static char *_log_level2str[]   = {"NOLOG","FATAL","ERR","WARN","NOTICE","INFO","ALL"};
static char *_log_level2str[]   = {"O","F","E","W","N","I","A"};
static char *_log_level2color[] = {"","\33[31m","\33[35m","\33[33m","\33[36m","",""};

#define _LOG_ERROR(level,format,args...) do{\
    char _errbuf[128], _log_outbuf[8192], _formatbuf[1024];\
    strerror_r(errno, _errbuf, sizeof(_errbuf));\
    snprintf(_formatbuf, sizeof(_formatbuf), "%s: %s", _errbuf, format);\
    int _log_outbuf_ret = zc_log_str(_log_level2color[level], log->logwhole,level,__FILE__,__LINE__,_log_outbuf,sizeof(_log_outbuf),_formatbuf,##args);\
    zcLogItem *_item = zc_log_find_item(log,level);\
    zc_log_write_real(log,_item,_log_outbuf, _log_outbuf_ret);\
    }while(0)


#define _LOG_INFO(level,format,args...) do{\
    char _log_outbuf[8192];\
    int _log_outbuf_ret = zc_log_str(_log_level2color[level], log->logwhole,level,__FILE__,__LINE__,_log_outbuf,sizeof(_log_outbuf),format,##args);\
    zcLogItem *_item = zc_log_find_item(log,level);\
    zc_log_write_real(log,_item,_log_outbuf, _log_outbuf_ret);\
    }while(0)

#define _LOG_STDERR(format,args...) do{\
    char _errbuf[128], _log_outbuf[8192], _formatbuf[1024];\
    strerror_r(errno, _errbuf, sizeof(_errbuf));\
    snprintf(_formatbuf, sizeof(_formatbuf), "%s: %s", _errbuf, format);\
    int _log_outbuf_ret = zc_log_str(NULL,0,ZC_LOG_FATAL,__FILE__,__LINE__,_log_outbuf,sizeof(_log_outbuf),_formatbuf,##args);\
    zc_log_write_fd(STDERR_FILENO,_log_outbuf, _log_outbuf_ret);\
    }while(0)


static zcLogItem* zc_log_find_item(zcLog *log, int loglevel);
static int zc_log_write_real(zcLog *log, zcLogItem *item, char *buffer, int wlen);
static int zc_log_write_fd(int fd, char *buffer, int len);
static int zc_log_str(char *color, int logwhole, int level, const char *file, int line, char *buffer, int blen, const char *format, ...);

static unsigned long
_gettid()
{
#ifdef _WIN32
    return (unsigned long)pthread_self().p;
#else
    return (unsigned long)pthread_self();
#endif
}

typedef struct _logfile_item {
    int64_t id[2];
    char    name[128];
}LogFileItem;


zcLog   *_zc_log;

static int
zc_logitem_tcp_new_connect(zcLogItem *item)
{
    if (item->fd > 0)
        close(item->fd);
    
    item->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (item->fd == -1) {
        _LOG_STDERR("log socket create error");
        return ZC_ERR;
    }

    int ret;
    do { 
        ret = connect(item->fd, (struct sockaddr*)&item->sin, sizeof(item->sin));
    } while (ret == -1 && errno == EINTR);

    if (ret != -1)
        return ZC_OK;
    _LOG_STDERR("log socket connect error");
    return ZC_ERR;
}

static int
zc_log_openfile(zcLog *log, zcLogItem *item)
{
    char filename[PATH_MAX] = {0};

    if (log->suffix == ZC_LOG_SUFFIX_PID) {
        zcDateTime dt;
        zc_datetime_init_now(&dt);
        snprintf(filename, PATH_MAX, "%s.%d%02d%02d.%02d%02d%02d.%d", item->filename, 
                dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second, getpid());
    }else{
        strcpy(filename, item->filename);
    }
    
    item->fd = open(filename, O_WRONLY|O_CREAT|O_APPEND, 0644);
    if (item->fd == -1) {
        ZCFATAL("open log %s error! %s\n", item->filename, strerror(errno));
        return -1;
    }
    return 0; 
}


static int
zc_logitem_init(zcLogItem *item, zcLog *log, const char *filename, int level, int dup)
{
    memset(item, 0, sizeof(zcLogItem));

    item->last_rotate_time = time(NULL);
    strncpy(item->filename, filename, PATH_MAX-1);
    item->protocol = ZC_LOG_FILE;
    item->level = level;
    item->dup   = dup;
 
    if (strcmp(filename, "stdout") == 0) {
        item->fd = STDOUT_FILENO;
        return ZC_OK;
    } 

    // file://path, syslog://name, tcp://ip:port, udp://ip:port
    if (strstr(filename, "://") == NULL) {
        if (strncmp(filename, "tcp://", 6) == 0) {
            item->protocol = ZC_LOG_TCP;
            char *sp = strchr(filename+6, ':');
            strncpy(item->host, filename+6, sizeof(item->host)-1);
            item->port = atoi(sp+1);

            item->sin.sin_family = AF_INET;
            item->sin.sin_port = htons((short)(item->port));
            if (item->host[0] == 0) {
                item->sin.sin_addr.s_addr = htonl(INADDR_ANY);
            }else{
                item->sin.sin_addr.s_addr = inet_addr(item->host);
            }    

            if (zc_logitem_tcp_new_connect(item) != ZC_OK) {
                return ZC_ERR;
            }
            return ZC_OK;
        }else if (strncmp(filename, "udp://", 6) == 0) {
            item->protocol = ZC_LOG_UDP;
            char *sp = strchr(filename+6, ':');
            strncpy(item->host, filename+6, sizeof(item->host)-1);
            item->port = atoi(sp+1);
            
            item->sin.sin_family = AF_INET;
            item->sin.sin_port = htons((short)(item->port));
            if (item->host[0] == 0) {
                item->sin.sin_addr.s_addr = htonl(INADDR_ANY);
            }else{
                item->sin.sin_addr.s_addr = inet_addr(item->host);
            }    

            item->fd = socket(AF_INET, SOCK_DGRAM, 0);
            if (item->fd == -1)
                return ZC_ERR;

            return ZC_OK;
        }else if (strncmp(filename, "syslog://", 9) == 0) {
            item->protocol = ZC_LOG_SYSLOG;
            return ZC_OK;
        }else if (strncmp(filename, "file://", 7) == 0) {
            item->protocol = ZC_LOG_FILE;
            strncpy(item->filename, filename+7, PATH_MAX);
        }else{
            item->protocol = ZC_LOG_FILE;
            //_LOG_STDERR("log filename error:%s\n", filename);
            //exit(-1);
            //return ZC_ERR;
        }
    }

    // file
    strncpy(item->filename, filename, PATH_MAX);
    if (strcmp(filename, "stdout") == 0) {
        item->fd = STDOUT_FILENO;
    }else{
        zc_log_openfile(log, item);
    } 
    return ZC_OK;
}

zcLog*  
zc_log_new(const char *filename, int loglevel)
{
    zcLog   *log = (zcLog*)zc_malloc(sizeof(zcLog));
    zc_log_init(log, filename, loglevel);

    return log;
}

void
zc_log_delete(void *log)
{
    zc_log_destroy(log);
    zc_free(log);
}


int  
zc_log_init(zcLog *log, const char *filename, int loglevel)
{
    if (loglevel > ZC_LOG_ALL || loglevel < 0) {
        ZCFATAL("loglevel error:%d\n", loglevel);
        return ZC_ERR;
    }
    memset(log, 0, sizeof(zcLog));
    //zcLogItem *item = &log->items[loglevel];
    //item->last_rotate_time = time(NULL);
    //strncpy(item->filename, filename, PATH_MAX-1);
    //

    //strncpy(log->filename[loglevel], filename, PATH_MAX-1);
    log->loglevel = loglevel;
    log->maxsize  = 0; 
    log->maxtime  = 0;
    log->logwhole = ZC_FALSE;
    log->suffix   = ZC_LOG_SUFFIX_TIME;
    log->check_interval = 5;

    zcLogItem *item = &log->items[loglevel];
    zc_logitem_init(item, log, filename, loglevel, 0);

    if (pthread_mutex_init(&log->lock, NULL) != 0) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        fprintf(stderr, "mutex init error: %s\n", errbuf);
        exit(EXIT_FAILURE);
    } 
    _zc_log = log;
    return ZC_OK;
}

void
zc_log_destroy(void *x)
{
    zcLog   *log = (zcLog*)x;
    int i;
    for (i=0; i<ZC_LOG_ALL; i++) {
        zcLogItem *item = &log->items[i];
        if (item->fd > 2 && item->fd != STDOUT_FILENO) {
            close(item->fd);
        } 
    }
}


int
zc_log_file(zcLog *log, const char *filename, int loglevel, int dup)
{
    if (loglevel > ZC_LOG_ALL || loglevel < 0) {
        ZCFATAL("loglevel error:%d\n", loglevel);
        return ZC_ERR;
    }
    zcLogItem *item;
    // 不能在不同level里有重复的文件名
    int i;
    for (i=loglevel; i<ZC_LOG_ALL; i++) {
        item = &log->items[i];
        if (item->filename[0] != 0 && strcmp(item->filename, filename) == 0) {
            ZCWARN("logfile %s exist, skip\n", filename);
            return ZC_OK;
        }
    }
    item = &log->items[loglevel];
    if (item->filename[0] != 0 && strcmp(item->filename, filename) == 0) {
        ZCNOTICE("logfile %s exist, skip\n", filename);
        return ZC_OK;
    }

    if (item->fd > 0) {
        ZCWARN("log %d is opened\n", loglevel);
        return ZC_ERR;
    }
    
    return zc_logitem_init(item, log, filename, loglevel, dup);

    /*item->level = loglevel;
    item->dup   = dup;
    item->protocol = ZC_LOG_FILE;
 
    // file://path, syslog://name, tcp://ip:port, udp://ip:port
    if (strstr(filename, "://") == NULL) {
        if (strncmp(filename, "tcp://", 6) == 0) {
            item->protocol = ZC_LOG_TCP;
            char *sp = strchr(filename+6, ':');
            strncpy(item->host, filename+6, sizeof(item->host)-1);
            item->port = atoi(sp+1);

            item->sin.sin_family = AF_INET;
            item->sin.sin_port = htons((short)(item->port));
            if (item->host[0] == 0) {
                item->sin.sin_addr.s_addr = htonl(INADDR_ANY);
            }else{
                item->sin.sin_addr.s_addr = inet_addr(item->host);
            }    

            if (zc_log_tcp_new_connect(log, item) != ZC_OK) {
                return ZC_ERR;
            }
            return ZC_OK;
        }else if (strncmp(filename, "udp://", 6) == 0) {
            item->protocol = ZC_LOG_UDP;
            char *sp = strchr(filename+6, ':');
            strncpy(item->host, filename+6, sizeof(item->host)-1);
            item->port = atoi(sp+1);
            
            item->sin.sin_family = AF_INET;
            item->sin.sin_port = htons((short)(item->port));
            if (item->host[0] == 0) {
                item->sin.sin_addr.s_addr = htonl(INADDR_ANY);
            }else{
                item->sin.sin_addr.s_addr = inet_addr(item->host);
            }    

            item->fd = socket(AF_INET, SOCK_DGRAM, 0);
            if (item->fd == -1)
                return ZC_ERR;

            return ZC_OK;
        }else if (strncmp(filename, "syslog://", 9) == 0) {
            item->protocol = ZC_LOG_SYSLOG;
            return ZC_OK;
        }else if (strncmp(filename, "file://", 7) == 0) {
            item->protocol = ZC_LOG_FILE;
            strncpy(item->filename, filename+7, PATH_MAX);
        }
    }

    // file
    strncpy(item->filename, filename, PATH_MAX);
    item->last_rotate_time = time(NULL);
    if (strcmp(filename, "stdout") == 0) {
        item->fd = STDOUT_FILENO;
    }else{
        zc_log_openfile(log, item);
    } 
   
    return ZC_OK;*/
}

void
zc_log_whole(zcLog *log, int flag)
{
    log->logwhole = flag;
}

void
zc_log_set_prefix(zcLog *log, char *prefix)
{
    if (NULL == prefix) {
        log->logprefix[0] = 0;
        return;
    }
    snprintf(log->logprefix, sizeof(log->logprefix), prefix);
}

int
zc_log_rotate_size(zcLog *log, int logsize, int logcount)
{
    log->maxsize = logsize;
    log->count   = logcount % 1000;
    log->rotate_type = ZC_LOG_ROTATE_SIZE;

    return ZC_OK;
}

int    
zc_log_rotate_time(zcLog *log, int logtime, int logcount)
{
    log->maxtime = logtime;
    log->count   = logcount % 1000;
    log->rotate_type = ZC_LOG_ROTATE_TIME;
    return ZC_OK;
}

int
zc_log_rotate_timeat(zcLog *log, int day, int hour, int min, int logcount)
{
    log->timeat[0] = day;
    log->timeat[1] = hour;
    log->timeat[2] = min;
    log->count = logcount % 1000;
    log->rotate_type = ZC_LOG_ROTATE_TIMEAT;
    return ZC_OK;
}

int    
zc_log_rotate_watch(zcLog *log)
{
    //log->maxtime = logtime;
    //log->count   = logcount % 1000;
    log->rotate_type = ZC_LOG_ROTATE_WATCH;
    return ZC_OK;
}

void    
zc_log_rotate_no(zcLog *log)
{
    log->rotate_type = ZC_LOG_ROTATE_NO;
}

/**
 * 查询大于指定日志级别的所有的日志文件
 */
static zcLogItem*
zc_log_find_item(zcLog *log, int loglevel)
{
    int i;
    zcLogItem *item = NULL;
    for (i=loglevel; i<=ZC_LOG_ALL; i++) {
        if (log->items[i].fd > 0) {
            item = &log->items[i];
            break;
        }
    }
    return item;
}

static int
zc_log_write_fd(int fd, char *buffer, int wlen)
{
    int rwlen = wlen;
    int wrn   = 0;
    int ret;
    
    while (rwlen > 0) {
        ret = write(fd, buffer + wrn, rwlen);
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }else{
                break;
            }
        }
        rwlen -= ret;
        wrn   += ret;
    }
    return wrn;
}

static int
zc_log_write_real(zcLog *log, zcLogItem *item, char *buffer, int wlen)
{
    int rwlen = wlen;
    int wrn   = 0;
    int ret;
    //int fd = -1;
    //int i = 0;
    //zcLogItem *item = zc_log_find_item(log, loglevel);
    if (NULL == item) {
        return ZC_ERR;
    }

    if (item->protocol == ZC_LOG_FILE) {
        wrn = zc_log_write_fd(item->fd, buffer, wlen);
    }else if (item->protocol == ZC_LOG_TCP) {
        while (rwlen > 0) {
            ret = write(item->fd, buffer + wrn, rwlen);
            if (ret == -1) {
                if (errno == EINTR) {
                    continue;
                }else{
                    if (zc_logitem_tcp_new_connect(item) < 0) {
                        return ZC_ERR;
                    }
                    continue;
                }
            }
            rwlen -= ret;
            wrn   += ret;
        }
    }else if (item->protocol == ZC_LOG_UDP) {
        socklen_t sa_len = sizeof(struct sockaddr_in);
        while (rwlen > 0) {
            int sendlen = rwlen>512?512:rwlen;
            ret = sendto(item->fd, buffer+wrn, sendlen, 0, (struct sockaddr*)&(item->sin), sa_len);
            if (ret == -1 && errno == EINTR)
                continue;
            if (ret == -1)
                break;
            wrn += ret; 
            rwlen -= ret;
        }
    }else if (item->protocol == ZC_LOG_SYSLOG) {
    }else{
        fprintf(stderr, "no protocol:%d", item->protocol);
    }
    return wrn;
}

static int
zc_log_write_real_dup(zcLog *log, int loglevel, char *buf, int len)
{
    int i;
    zcLogItem *item = NULL;
    for (i=loglevel; i<=ZC_LOG_ALL; i++) {
        if (log->items[i].fd > 0) {
            item = &log->items[i];
            zc_log_write_real(log, item, buf, len);
            if (item->dup == 0)
                break;
        }
    }
    return ZC_OK;
}


/*static int 
compare_int (const void *a, const void *b) 
{ 
    return *(int *)a - *(int *)b; 
}*/
/*static int 
compare_int64 (const void *a, const void *b) 
{ 
    return *(int64_t *)a - *(int64_t *)b; 
}*/

static int 
compare_logfileitem (const void *a, const void *b) 
{ 
    LogFileItem *a1 = (LogFileItem*)a;
    LogFileItem *b1 = (LogFileItem*)b;

    int ret;

    ret = a1->id[0] - b1->id[0];
    if (ret != 0)
        return ret;

    return a1->id[1] - b1->id[1];
}

/**
 * 日志文件滚动
 * NOTE: 日志文件后缀格式为pid的不会滚动
 * 首先查找日志文件目录中的所有的日志文件，并判断统计日志文件数和对日志文件排序
 * 如果日志文件数超过了允许保留的日志文件数，则删除最早创建的日志文件
 * 切分日志时:
 * 对于以num为后缀的日志文件，会将所有的日志文件后缀的num+1, 将本次切分的日志文件后缀设置为1.
 *      num的规则: 按时间，从大到小排序, 创建时间越久，num越大
 * 按时间切分日志时，会将创建最早的日志文件删除，并将本次切分的日志文件按格式命名
 *      命令规则: log.yyyymmdd.hhMMSS[.num] 
 *                如果一秒出现超过1个日志文件，则会出现num，且从1开始递增, 否则不会出现num
 */
static int
zc_log_do_rotate(zcLog *log, int index)
{
    //uint32_t    timenow = time(NULL);    
    /*if (timenow - log->last_rotate_time < 1) {
        return ZC_ERR;
    }*/

    zcLogItem *item = &log->items[index];

    if (log->suffix == ZC_LOG_SUFFIX_PID) {
        // 后缀为pid的日志，不判断日志文件数是否超过允许的数量
        // FIXME: 为什么呢？
        //zc_log_openfile(log, item);
        //return 0;
        goto ROTATE;
    }

    int  i;
    char *logname;
    char logdir[PATH_MAX];
    // find logfile directory
    logname = strrchr(item->filename, '/');
    if (logname == NULL) {
        logname = item->filename;
        strcpy(logdir, ".");
    }else{
        i = 0; 
        while (&item->filename[i] < logname) {
            logdir[i] = item->filename[i];
            i++;
            //_LOG_INFO("copy %c\n", logdir[i]);
        }
        logdir[i] = 0;
        logname++;
    }
    //_LOG_INFO(item->level, "logdir:%s, logname:%s\n", logdir, logname);
    
    char log_prefix[256];
    strcpy(log_prefix, logname);
    strcat(log_prefix, ".");

    int log_prefix_len = strlen(log_prefix);
    LogFileItem logs[1000]; 
    DIR *mydir;
    struct dirent *nodes;

    memset(logs, 0, sizeof(logs));

    mydir = opendir(logdir);
    if (NULL == mydir) {
        _LOG_ERROR(item->level, "open dir %s error\n", logdir);
        exit(EXIT_FAILURE);
    }
    i = 0;
    //int64_t logid;
    char buf[32];
    char *s;
    int  blen;
    // find all logfile
    // 查找所有的日志文件，并按时间排序，如果超过了可能保留的日志文件数，则删除最早的日志文件
    while ((nodes = readdir(mydir)) != NULL) {
        if (strncmp(nodes->d_name, log_prefix, log_prefix_len) == 0 && \
            isdigit(nodes->d_name[log_prefix_len])){
            blen = 0;
            s = &nodes->d_name[log_prefix_len];
            int idx = 0;
            while (*s) {
                if (isdigit(*s)) {
                    buf[blen] = *s;
                    blen++;
                }else if (*s == '.') {
                    buf[blen] = 0;
                    logs[i].id[idx] = strtoll(buf, NULL, 10);
                    blen = 0;
                    idx++;
                    if (idx >= 2)
                        break;
                }
                s++;
            }
            if (idx <= 1) {
                buf[blen] = 0;
                logs[i].id[idx] = strtoll(buf, NULL, 10);
            }
            //_LOG_INFO(ZC_LOG_INFO, "name:%s, id:%d %d, size:%ld\n", 
            //    nodes->d_name, (int)logs[i].id[0], (int)logs[i].id[1], sizeof(logs[i].name));
            snprintf(logs[i].name, sizeof(logs[i].name), "%s", nodes->d_name);
            i++;
        }
    }
    closedir(mydir);

    //qsort(logs, i, sizeof(int64_t), compare_int64);  
    qsort(logs, i, sizeof(LogFileItem), compare_logfileitem);  

    char filename[PATH_MAX];
    char newfilename[PATH_MAX];
            
    //sprintf(filename, "%s.%d%02d%02d.%02d%02d%02d", item->filename, i);
    if (log->suffix == ZC_LOG_SUFFIX_TIME) {
        if (log->count > 0 && i >= log->count) {
            sprintf(filename, "%s", logs[0].name);
            if (unlink(filename) == -1) {
                _LOG_ERROR(item->level, "unlink %s error\n", filename);
            }
        }
        time_t timenow = time(NULL);
        struct tm timestru;
        localtime_r(&timenow, &timestru);

        sprintf(newfilename, "%s.%d%02d%02d.%02d%02d%02d", item->filename, 
                timestru.tm_year+1900, timestru.tm_mon+1, timestru.tm_mday,
                timestru.tm_hour, timestru.tm_min, timestru.tm_sec);

        char newfile2[PATH_MAX] = {0};
        strcpy(newfile2, newfilename);
        int newfilei = 1;
        while (zc_isfile(newfile2)) {
            snprintf(newfile2, sizeof(newfile2), "%s.%d", newfilename, newfilei);
            newfilei++;
        }
        if (rename(item->filename, newfile2) == -1) {
            _LOG_ERROR(item->level, "rename %s to %s error\n", filename, newfile2);
        }
    }else{
        // 日志后缀是数量
        // 数量的规则是按时间，从大到小排序，时间越长，num越大
        // 所以，所有文件的num + 1
        for (;i > 0; i--) {
            sprintf(filename, "%s.%d", item->filename, i);
            if (i >= log->count) {
                if (unlink(filename) == -1) {
                    _LOG_ERROR(item->level, "unlink %s error\n", filename);
                    //exit(EXIT_FAILURE);
                }
            }else{
                sprintf(newfilename, "%s.%d", item->filename, i+1);
                if (rename(filename, newfilename) == -1) {
                    _LOG_ERROR(item->level, "rename %s to %s error\n", filename, newfilename);
                    //exit(EXIT_FAILURE);
                }
            }
        }
        // 新文件num为1
        sprintf(newfilename, "%s.%d", item->filename, 1);
        if (rename(item->filename, newfilename) == -1) {
            _LOG_ERROR(item->level, "rename %s to %s error\n", filename, newfilename);
            //exit(EXIT_FAILURE);
        }
    }
    
ROTATE:
    if (item->fd > 2)
        close(item->fd);
    zc_log_openfile(log, item);

    return ZC_OK;
}

int
zc_log_check_rotate(zcLog *log, int loglevel)
{
    zcLogItem *item = zc_log_find_item(log, loglevel);
    if (NULL == item)
        return ZC_ERR;
    //_LOG_INFO("check log ...\n");
    //uint32_t    timenow = time(NULL);    
    time_t    timenow = time(NULL);    
    if (item->fd <= 2) { // not stdin 0, stdout 1, stderr 2
        return 0; 
    }
    if (timenow - item->last_check_time < log->check_interval) {
        //_LOG_INFO("not need check. %u %u %d\n", timenow, 
        //    log->last_check_time, log->check_interval);
        return 0;
    }
    //_LOG_INFO(item->level, "rotate type:%d\n", log->rotate_type); 
    int ret;
    struct stat fs;

    pthread_mutex_lock(&log->lock);
    switch(log->rotate_type) {
    case ZC_LOG_ROTATE_SIZE:
        //_LOG_INFO("check size.\n");
        ret = fstat(item->fd, &fs);
        if (ret == -1) {
            _LOG_ERROR(item->level, "fstat error");
        }else{
            //_LOG_INFO("file size:%d, maxsize:%d\n", (int)fs.st_size, log->maxsize);
            if (fs.st_size >= log->maxsize) {
                zc_log_do_rotate(log, item->level);
                item->last_rotate_time = timenow;
            }
        }
        break;
    case ZC_LOG_ROTATE_TIME:
        //_LOG_INFO("check time. %d, %d\n", timenow - log->last_rotate_time, log->maxtime);
        if (timenow - item->last_rotate_time >= log->maxtime) {
            zc_log_do_rotate(log, item->level);
            item->last_rotate_time = timenow;
        }
        break;
    case ZC_LOG_ROTATE_TIMEAT: 
        if (timenow - item->last_rotate_time > 60) {
            struct tm timestru;
            localtime_r(&timenow, &timestru);

            int t[3] = {timestru.tm_mday, timestru.tm_hour, timestru.tm_min};
            int i;
            for (i=0; i<3; i++) {
                if (log->timeat[i] < 0) {
                    continue;
                }
                if (log->timeat[i] != t[i])
                    break;
            }
            if (i == 3) {
                zc_log_do_rotate(log, item->level);
                item->last_rotate_time = timenow;
            }
        }
        break;
    case ZC_LOG_ROTATE_WATCH:
        break;
    case ZC_LOG_ROTATE_REOPEN:
        // 1分钟重新打开文件一次
        if (timenow - item->last_rotate_time >= 60) {
            ZCINFO("reopen log file:%s", item->filename);
            if (item->fd > 2)
                close(item->fd);
            /*item->fd = open(item->filename, O_CREAT|O_WRONLY|O_APPEND, 0644);
            if (-1 == item->fd) {
                _LOG_ERROR(item->level, "open log file %s error\n", item->filename);
                exit(EXIT_FAILURE);
            }*/
            zc_log_openfile(log, item);
            item->last_rotate_time = timenow;
        }
        break;
    }
    item->last_check_time = timenow;
    pthread_mutex_unlock(&log->lock);

    return 0;
}


int     
zc_log_write(zcLog *log, int level, const char *file, int line, const char *format, ...)
{
    char    buffer[8192];
    //char    color[16] = {0};
    //char    levelstr[16] = {0};
    char    *color = "";
    char    *levelstr = _log_level2str[level];
    va_list arg;
    int     maxsize = sizeof(buffer)-6; // const. 4+1+1
    int     maxlen = sizeof(buffer)-6; // ascii color end 
    int     ret, wlen = 0;
    time_t  timenow;

    struct tm   timestru;
    struct timeb tmb;
    time(&timenow);
    localtime_r(&timenow, &timestru);
    ftime(&tmb);
    
    char *rsp = strrchr(file, '/');
    if (rsp != NULL) {
        file = rsp + 1;
    }

    zcLogItem *item = zc_log_find_item(log, level);
    if (NULL == item) {
        return ZC_ERR;
    }
    if (item->fd == STDOUT_FILENO) {
        color = _log_level2color[level];
    }

    if (color[0] == 0) {
        ret = snprintf(buffer, maxlen, "%d%02d%02d %02d%02d%02d.%03d %d,%lu %s:%d [%s] %s",
                    timestru.tm_year-100, timestru.tm_mon+1, timestru.tm_mday,
                    timestru.tm_hour, timestru.tm_min, timestru.tm_sec, tmb.millitm,
                    (int)getpid(), _gettid(), file, line, levelstr, log->logprefix);
    }else{
        ret = snprintf(buffer, maxlen, "%s%d%02d%02d %02d%02d%02d.%03d %d,%lu %s:%d [%s] %s",
                    color, timestru.tm_year-100, timestru.tm_mon+1, timestru.tm_mday,
                    timestru.tm_hour, timestru.tm_min, timestru.tm_sec, tmb.millitm,
                    (int)getpid(), _gettid(), file, line, levelstr, log->logprefix);
    }

    maxlen -= ret;
    wlen = ret;

    if (log->logwhole) {
        zc_log_write_real_dup(log, level, buffer, wlen);
            
        char *wbuf = NULL;
        va_start (arg, format);
        ret = vasprintf (&wbuf, format, arg);
        va_end (arg);
       
        int wbuflen = 0;
        if (wbuf != NULL) {
            wbuflen = strlen(wbuf);
            zc_log_write_real_dup(log, level, wbuf, wbuflen);

            if (wbuf[wbuflen-1] != '\n') {
                zc_log_write_real_dup(log, level, "\n", 1);
                wbuflen += 1;
            }

            free(wbuf);
        }
        if (color[0] != 0) {
            zc_log_write_real_dup(log, level, "\33[0m", 4);
            wbuflen += 4;
        }

        zc_log_check_rotate(log, level);
        return wlen + wbuflen;
    }else{
        va_start (arg, format);
        ret = vsnprintf (buffer+wlen, maxlen, format, arg);
        wlen += ret;
        va_end (arg);

        if (wlen > maxsize) {
            wlen = maxsize;
            buffer[wlen] = '\n';
            wlen++;
        }else{
            if (buffer[wlen-1] != '\n') {
                buffer[wlen] = '\n';
                wlen += 1;
            } 
        }

        if (color[0] != 0) {
            char *endtmp = "\33[0m";
            strcpy(buffer+wlen, endtmp);  
            wlen += strlen(endtmp);
        }
        buffer[wlen] = 0;
        //printf("wlen:%d, %s\n", wlen, buffer);
        //ret = zc_log_write_real(log, item, buffer, wlen);
        ret = zc_log_write_real_dup(log, level, buffer, wlen);
        zc_log_check_rotate(log, level);
        return ret;
    }
}

void 
zc_log_flush(zcLog *log)
{
    int i;
    for (i=0; i<ZC_LOG_ALL; i++) {
#ifdef _WIN32
        _commit(log->items[i].fd);
#else
        fsync(log->items[i].fd);
#endif
    }
}


/*static int     
zc_log_time_str(char *buffer, int blen)
{
    time_t      timenow;
    struct tm   timestru;
    struct timeb tmb;

    time(&timenow);
    localtime_r(&timenow, &timestru);
    ftime(&tmb);
    
    snprintf(buffer, blen, "%d%02d%02d %02d:%02d:%02d.%03d",
            timestru.tm_year-100, timestru.tm_mon+1, timestru.tm_mday,
            timestru.tm_hour, timestru.tm_min, timestru.tm_sec, tmb.millitm);

    return 0;
}

static int
zc_log_prefix_str(int level, const char *file, int line, char *buffer, int blen)
{
    char    levelstr[16] = {0};
    int     ret;
    time_t  timenow;

    struct tm   timestru;
    struct timeb tmb;
    time(&timenow);
    localtime_r(&timenow, &timestru);
    ftime(&tmb);
    
    char *rsp = strrchr(file, '/');
    if (rsp != NULL) {
        file = rsp + 1;
    }

    ret = snprintf(buffer, blen, "%d%02d%02d %02d:%02d:%02d.%03d %d,%lu %s:%d [%s] ",
                timestru.tm_year-100, timestru.tm_mon+1, timestru.tm_mday,
                timestru.tm_hour, timestru.tm_min, timestru.tm_sec, tmb.millitm,
                (int)getpid(), _gettid(), file, line, levelstr);

    return ret;
}*/

static int     
zc_log_str(char *color, int logwhole, int level, const char *file, int line, char *buffer, int blen, const char *format, ...)
{
    va_list arg;
    int     maxsize = blen-6; // 4+1+1, color_end+\n+\0
    int     ret, wlen = 0;
    time_t  timenow;

    struct tm   timestru;
    struct timeb tmb;
    time(&timenow);
    localtime_r(&timenow, &timestru);
    ftime(&tmb);
    
    char *rsp = strrchr(file, '/');
    if (rsp != NULL) {
        file = rsp + 1;
    }

    int start = 0;
        //char *color = _log_level2color[level];
    if (color && color[0] != 0) {
        strcpy(buffer, color);
        start = strlen(color);
    }
    unsigned long tid = _gettid();
    ret = snprintf(buffer+start, blen-start, "%d%02d%02d %02d%02d%02d.%03d %d,%lu %s:%d [%s] ",
                timestru.tm_year-100, timestru.tm_mon+1, timestru.tm_mday,
                timestru.tm_hour, timestru.tm_min, timestru.tm_sec, tmb.millitm,
                (int)getpid(), tid, file, line, _log_level2str[level]);
    ret += start;
    wlen = ret;
    if (logwhole) {
        char *wbuf = NULL;
        va_start (arg, format);
        ret = vasprintf (&wbuf, format, arg);
        va_end (arg);
       
        if (wbuf != NULL) {
            int wbuflen = strlen(wbuf);
            strcpy(buffer+wlen, wbuf);
            wlen += wbuflen;

            if (wbuf[wbuflen-1] != '\n') {
                *(buffer+wlen) = '\n';
                wlen++;
            }
            free(wbuf);
        }
        if (start > 0) {
            strcpy(buffer+wlen, "\33[0m");
            wlen += 4;
        }
        return wlen;
    }else{
        va_start (arg, format);
        ret = vsnprintf (buffer+wlen, blen-wlen, format, arg);
        wlen += ret;
        va_end (arg);

        if (wlen > maxsize) {
            wlen = maxsize;
            buffer[wlen] = '\n';
            wlen++;
        }else{
            if (buffer[wlen-1] != '\n') {
                buffer[wlen] = '\n';
                wlen += 1;
            } 
        }

        if (start > 0) {
            strcpy(buffer+wlen, "\33[0m");  
            wlen += 4;
        }
        buffer[wlen] = 0;
        return wlen;
    }
}


