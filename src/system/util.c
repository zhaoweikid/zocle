#if !defined( _WIN32) //&& defined(__linux)
#include <zocle/system/util.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#ifdef __linux
#include <sys/prctl.h>
#endif
#include <sys/types.h>
#include <errno.h>
#include <sys/resource.h>
#include <zocle/base/defines.h>
#include <zocle/log/logfile.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>

//jmp_buf  _zc_jmp_buf;

int zc_coredump_enable(uint64_t size)
{
    struct rlimit rlim;
#ifdef __linux
    if (prctl(PR_SET_DUMPABLE, 1) < 0) {
        ZCWARN("prctl error!\n");
        return ZC_ERR; 
    }   
#endif
    if (0 != getrlimit(RLIMIT_CORE, &rlim)) {
        ZCWARN("get max coredump file size error: %s\n", strerror(errno));
        return ZC_ERR; 
    }   
    
    struct rlimit rlim_new;
    if (size == 0) {
        rlim_new.rlim_cur = rlim_new.rlim_max = RLIM_INFINITY;
    }else{
        rlim_new.rlim_cur = rlim_new.rlim_max = size;
    }
    //rlim.rlim_cur = rlim.rlim_max;

    if (0 != setrlimit(RLIMIT_CORE, &rlim_new)) {
        rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
        ZCWARN("set max coredump file size: %llu\n", (unsigned long long)rlim.rlim_max);
        setrlimit(RLIMIT_CORE, &rlim_new);
        return ZC_ERR; 
    } 
    if ((getrlimit(RLIMIT_CORE, &rlim) != 0) || rlim.rlim_cur == 0) {
        ZCERROR("failed to ensure corefile creation\n");
        return ZC_ERR;
    }
    return ZC_OK;
}

int zc_filelimit(int maxfiles)
{
    struct rlimit rlim;
    if (getrlimit(RLIMIT_NOFILE, &rlim) != 0) { 
        ZCERROR("failed to getrlimit number of files\n");
        return ZC_ERR;
    } else {
        if (rlim.rlim_cur < maxfiles)
            rlim.rlim_cur = maxfiles;
        if (rlim.rlim_max < rlim.rlim_cur)
            rlim.rlim_max = rlim.rlim_cur;
        if (setrlimit(RLIMIT_NOFILE, &rlim) != 0) { 
            ZCERROR("failed to set rlimit for open files. Try running as root or requesting smaller maxconns value.\n");
            return ZC_ERR;
        }       
    }
    return ZC_OK;
}

int zc_daemon(const char *home)
{
    pid_t pid;
    struct rlimit rl; 
    //int i;
    int fd0, fd1, fd2;

    getrlimit(RLIMIT_NOFILE, &rl);

    if (0 > (pid = fork()))
        return ZC_ERR;
    else if (0 != pid)
        _exit(0);

    setsid();
    //chdir("/");
    if (home) {
        chdir(home);
    }
    umask(0);

    if (0 > (pid = fork()))
        return (-1);
    else if (0 != pid)
        _exit(0);

    // close 0,1,2
    int i;
    for (i = 0; i < 3; i++) {
        close(i);
    }

    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup2(fd0, 1);
    fd2 = dup2(fd0, 2);

    if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
        ZCERROR("daemon file descriptor error: %d %d %d\n", fd0, fd1, fd2);
    }

    return ZC_OK;
}

int zc_get_uid_by_name(const char *name)
{
    struct passwd *pw;

    pw = getpwnam(name);
    if (NULL == pw)
        return -1;
    else
        return pw->pw_uid;
}

int zc_set_uid_by_name(const char *name)
{
    int myuid;
    if (-1 == (myuid = zc_get_uid_by_name(name)))
        return -1;

    return setuid(myuid);
}

int zc_signal_set_default(void (*signal_exit_handler)(int))
{
    struct sigaction sigact;
    sigact.sa_handler = signal_exit_handler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;

    sigaction(SIGTERM, &sigact, NULL);
    sigaction(SIGQUIT, &sigact, NULL);
    sigaction(SIGINT, &sigact, NULL);
    sigaction(SIGILL , &sigact, NULL);
    sigaction(SIGABRT, &sigact, NULL);
    sigaction(SIGFPE , &sigact, NULL);
    sigaction(SIGBUS , &sigact, NULL);
    //sigaction(SIGSEGV, &ssigAct, NULL);

    return 0;
}

int zc_signal_set(int signo, void (*signal_exit_handler)(int))
{
    struct sigaction sigact;
    sigact.sa_handler = signal_exit_handler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;

    sigaction(signo, &sigact, NULL);

    return 0;
}

#endif

