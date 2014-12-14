#include <zocle/zocle.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>

#define removelogs() system("rm -rf test.log test.*.log* test.log.*")

int find_count(char* filename, char *s)
{
    FILE *fp = fopen(filename, "r");
    if (NULL == fp) {
        fprintf(stderr, "open file error:%s\n", strerror(errno));
        return -1;
    }

    char line[4096];  
    int count = 0;

    //printf("try get line ...\n");
    while (fgets(line, sizeof(line), fp) != NULL) {
        //printf("read line:%s\n", line);
        if (strstr(line, s) != NULL)
            count++;
    }
    fclose(fp);

    return count;
}

int64_t islogfile_suffix(char *name)
{
    char buf[128] = {0};
    int i = 0;

    ZCINFO("suffix:%s\n", name);
    while (*name) {
        if (!isdigit(*name) && *name != '.')
            return ZC_FALSE;
        buf[i] = *name;
        i++;
        name++;
    }
    buf[i] = 0;

    return strtoll(buf, NULL, 10);
}


int find_files(char *filename)
{
    DIR *mydir;
    struct dirent *nodes;
    char logdir[PATH_MAX];
    strcpy(logdir, ".");

    char prefix[256];
    int  prefix_len;
    sprintf(prefix, "%s.", filename);
    
    prefix_len = strlen(prefix);
    //int count = 0;
    int64_t logs[1000] = {0};
    mydir = opendir(logdir);
    if (NULL == mydir) {
        return -1;
    }   
    int i = 0;
    while ((nodes = readdir(mydir)) != NULL) {
        //printf("name:%s\n", nodes->d_name);
        if (strncmp(nodes->d_name, prefix, prefix_len) == 0) {
            int64_t logid = islogfile_suffix(&nodes->d_name[prefix_len]);
            if (logid == 0)
                continue;
            logs[i] = logid;    
            i++;
        }   
    }   
    closedir(mydir);

    return i;
}


int test_info()
{
    printf("test info ...\n");
    removelogs();
    zc_log_new("test.log", ZC_LOG_INFO);

    //strcpy(_zc_log->logprefix, "127.0.0.1:1100 ");

    int i;
    for (i=0; i<10; i++) {
        zc_log_set_prefix(_zc_log, "127.0.0.1:1100 ");
        ZCINFO("test info %d\n", i);
        ZCNOTICE("test notice %d\n", i);

        zc_log_set_prefix(_zc_log, NULL);
        ZCWARN("test warn %d\n", i);
        ZCERROR("test error %d\n", i);
    }
    zc_log_flush(_zc_log);
    zc_log_delete(_zc_log);

    int count;
    
    count = find_count("test.log", "[INFO]");
    assert(count == 10);
    
    count = find_count("test.log", "[NOTICE]");
    assert(count == 10);

    count = find_count("test.log", "[WARN]");
    assert(count == 10);

    count = find_count("test.log", "[ERR]");
    assert(count == 10);

    return 0;
}

int test_err()
{
    printf("test err ...\n");
    removelogs();
    zc_log_new("test.log", ZC_LOG_ERROR);
 
    int i;
    for (i=0; i<10; i++) {
        ZCINFO("test info %d\n", i);
        ZCNOTICE("test notice %d\n", i);
        ZCWARN("test warn %d\n", i);
        ZCERROR("test error %d\n", i);
    }
    zc_log_flush(_zc_log);
    zc_log_delete(_zc_log);

    int count;
    
    count = find_count("test.log", "[INFO]");
    assert(count == 0);
    
    count = find_count("test.log", "[NOTICE]");
    assert(count == 0);

    count = find_count("test.log", "[WARN]");
    assert(count == 0);

    count = find_count("test.log", "[ERR]");
    assert(count == 10);

    return 0;
}

int test_multi_log()
{
    printf("test multi log ...\n");
    removelogs();
    zc_log_new("test.log", ZC_LOG_INFO);
    zc_log_file(_zc_log, "test.err.log", ZC_LOG_ERROR, 0);
    zc_log_file(_zc_log, "test.warn.log", ZC_LOG_WARN, 0);
    zc_log_file(_zc_log, "test.notice.log", ZC_LOG_NOTICE, 0);
   
    int i;
    for (i=0; i<10; i++) {
        ZCINFO("test info %d\n", i);
        ZCNOTICE("test notice %d\n", i);
        ZCWARN("test warn %d\n", i);
        ZCERROR("test error %d\n", i);
    }
    zc_log_flush(_zc_log);
    zc_log_delete(_zc_log);

    int count;
    
    count = find_count("test.log", "[INFO]");
    assert(count == 10);
    
    count = find_count("test.log", "[NOTICE]");
    assert(count == 0);

    count = find_count("test.log", "[WARN]");
    assert(count == 0);

    count = find_count("test.log", "[ERR]");
    assert(count == 0);

    count = find_count("test.err.log", "[ERR]");
    assert(count == 10);

    count = find_count("test.warn.log", "[WARN]");
    assert(count == 10);

    count = find_count("test.notice.log", "[NOTICE]");
    assert(count == 10);

    return 0;
}

int test_multi_log_dup()
{
    printf("test multi log dup ...\n");
    removelogs();
    zc_log_new("test.log", ZC_LOG_INFO);
    zc_log_file(_zc_log, "test.notice.log", ZC_LOG_NOTICE, 1);
    zc_log_file(_zc_log, "test.warn.log", ZC_LOG_WARN, 1);
    zc_log_file(_zc_log, "test.err.log", ZC_LOG_ERROR, 0);
   
    int i;
    for (i=0; i<10; i++) {
        ZCINFO("test info %d\n", i);
        ZCNOTICE("test notice %d\n", i);
        ZCWARN("test warn %d\n", i);
        ZCERROR("test error %d\n", i);
    }
    zc_log_flush(_zc_log);
    zc_log_delete(_zc_log);

    int count;
    
    count = find_count("test.log", "[INFO]");
    assert(count == 10);
    
    count = find_count("test.log", "[NOTICE]");
    assert(count == 10);

    count = find_count("test.log", "[WARN]");
    assert(count == 10);

    count = find_count("test.log", "[ERR]");
    assert(count == 0);

    count = find_count("test.err.log", "[ERR]");
    assert(count == 10);

    count = find_count("test.warn.log", "[WARN]");
    assert(count == 10);

    count = find_count("test.notice.log", "[NOTICE]");
    assert(count == 10);

    count = find_count("test.notice.log", "[WARN]");
    assert(count == 10);

    return 0;
}

int test_rotate_size()
{    
    printf("test rotate size...\n");
    removelogs();
    zc_log_new("test.log", ZC_LOG_INFO);
    zc_log_rotate_size(_zc_log, 100000, 10); 
    _zc_log->check_interval = 0;
    int i;
    //char buf[1024];
    for (i=0; i<20000; i++) {
        ZCINFO("haha, my test, log rotate, at id:%d\n", i);
    }
    zc_log_delete(_zc_log);
    assert(find_files("test.log") == 10);

    return 0;
}

int test_rotate_time()
{    
    printf("test rotate time ...\n");
    removelogs();
    zc_log_new("test.log", ZC_LOG_INFO);
    zc_log_rotate_time(_zc_log, 1, 9); 
    _zc_log->check_interval = 0;
    int i;
    //char buf[1024];
    for (i=0; i<10000; i++) {
        ZCINFO("haha, my test, log rotate, at id:%d\n", i);
        if (i%1000 == 0)
            sleep(1);
    }
 
    zc_log_delete(_zc_log);

    assert(find_files("test.log") == 9);

    return 0;
}

int test_rotate_size_multi()
{    
    printf("test rotate size multi...\n");
    removelogs();
    zc_log_new("test.log", ZC_LOG_INFO);
    zc_log_file(_zc_log, "test.err.log", ZC_LOG_ERROR, 0);
    zc_log_file(_zc_log, "test.warn.log", ZC_LOG_WARN, 0);
    zc_log_file(_zc_log, "test.notice.log", ZC_LOG_NOTICE, 0);

    zc_log_rotate_size(_zc_log, 100000, 10); 
    _zc_log->check_interval = 0;
    int i;
    //char buf[1024];
    for (i=0; i<20000; i++) {
        ZCINFO("haha, my test, log rotate, at id:%d\n", i);
        ZCWARN("haha, my test, log rotate, at id:%d\n", i);
        ZCERROR("haha, my test, log rotate, at id:%d\n", i);
        ZCNOTICE("haha, my test, log rotate, at id:%d\n", i);
    }
 
    zc_log_delete(_zc_log);

    assert(find_files("test.log") == 10);
    assert(find_files("test.err.log") == 10);
    assert(find_files("test.warn.log") == 10);
    assert(find_files("test.notice.log") == 10);

    return 0;
}

int test_rotate_time_multi()
{    
    printf("test rotate time multi...\n");
    removelogs();
    zc_log_new("test.log", ZC_LOG_INFO);
    zc_log_file(_zc_log, "test.err.log", ZC_LOG_ERROR, 0);
    zc_log_file(_zc_log, "test.warn.log", ZC_LOG_WARN, 0);
    zc_log_file(_zc_log, "test.notice.log", ZC_LOG_NOTICE, 0);

    zc_log_rotate_time(_zc_log, 1, 9); 
    _zc_log->check_interval = 0;
    int i;
    //char buf[1024];
    for (i=0; i<15000; i++) {
        ZCINFO("haha, my test, log rotate, at id:%d\n", i);
        ZCWARN("haha, my test, log rotate, at id:%d\n", i);
        ZCERROR("haha, my test, log rotate, at id:%d\n", i);
        ZCNOTICE("haha, my test, log rotate, at id:%d\n", i);
        if (i%1000 == 0) {
            sleep(1);
        }
    }
 
    zc_log_delete(_zc_log);

    assert(find_files("test.log") == 9);
    assert(find_files("test.err.log") == 9);
    assert(find_files("test.warn.log") == 9);
    assert(find_files("test.notice.log") == 9);

    return 0;
}



int main(int argc, char *argv[])
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    system("rm -rf test.log*");

    test_info(); 
    test_err();
    test_multi_log();
    test_multi_log_dup();
    test_rotate_size();
    test_rotate_size_multi();
    
    system("rm -rf test.log*");
    test_rotate_time();
    test_rotate_time_multi();
    

    return 0;
}
