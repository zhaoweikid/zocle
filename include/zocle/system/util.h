#ifndef ZOCLE_SYSTEM_UTIL_H
#define ZOCLE_SYSTEM_UTIL_H

#include <stdio.h>
#include <stdint.h>
#include <setjmp.h>

#define zc_try(rc)  \
    rc=setjmp(_zc_jmp_buf);\
    if (rc == 0) 

#define zc_catch \
    else
    
#define zc_raise(rc)    longjmp(_zc_jmp_buf,rc)
#define zc_throw(rc)    longjmp(_zc_jmp_buf,rc)

//extern jmp_buf  _zc_jmp_buf;
jmp_buf  _zc_jmp_buf;

int zc_coredump_enable(uint64_t size);
int zc_filelimit(int maxfiles);
int zc_daemon(const char *home);
int zc_get_uid_by_name(const char *name);
int zc_set_uid_by_name(const char *name);
int zc_signal_set_default(void (*signal_exit_handler)(int));
int zc_signal_set(int signo, void (*signal_exit_handler)(int));

#endif
