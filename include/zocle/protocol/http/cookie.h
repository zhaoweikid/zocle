#ifndef ZOCLE_HTTP_COOKIE_H
#define ZOCLE_HTTP_COOKIE_H

#include <stdio.h>
#include <stdint.h>
#include <zocle/base/defines.h>
#include <zocle/str/string.h>

typedef struct {
	zcString key;
	zcString value;
	zcString domain;
	zcString path;
	uint32_t expires;
	bool     secure;
	bool     httponly;
}zcCookie;

zcCookie* zc_cookie_new(const char *);
void	  zc_cookie_delete(void *);
int		  zc_cookie_init(zcCookie*, const char *);
void	  zc_cookie_print(zcCookie*);

#endif
