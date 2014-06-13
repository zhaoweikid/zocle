#ifdef _WIN32
#ifndef ZOCLE_BASE_COMPACT_H
#define ZOCLE_BASE_COMPACT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <zocle/net/sockets.h>

#define S_ISLNK(X) 0

int 		fsync(int fd);
char*		strerror_r(int err, char *buf, int buflen);
struct tm* 	localtime_r (const time_t *timer, struct tm *result);
struct tm* 	gmtime_r (const time_t *timer, struct tm *result);
int 		asprintf(char **, const char *, ... );
int 		vasprintf(char **, const char *, va_list );
char* 		strptime(const char *s, const char *format, struct tm *tm);
/*const char*	inet_ntop(int af, const void *src, char *dst, socklen_t cnt);
int 		inet_pton(int af, const char *src, void *dst);*/

#ifndef sleep
#define sleep _sleep
#endif

#endif
#endif

