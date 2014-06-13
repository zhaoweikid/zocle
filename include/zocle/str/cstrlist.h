#ifndef ZOCLE_STR_CSTRLIST_H
#define ZOCLE_STR_CSTRLIST_H

#include <stdio.h>

typedef struct zc_cstrlist_t
{
	int  size;
	int  len;
	int  n;
	int  maxn;
	char *data;
}zcCStrList;

zcCStrList* zc_cstrlist_new(int size, int n);
void		zc_cstrlist_delete(void*);

void		zc_cstrlist_init(zcCStrList*, int size, int n);
void		zc_cstrlist_init_stack(zcCStrList*, char *buf, int size, int n);
void		zc_cstrlist_destroy(void*);

void		zc_cstrlist_clear(zcCStrList*);
const char*	zc_cstrlist_get(zcCStrList*, int pos);

int			zc_cstrlist_append(zcCStrList*, const char *s, int len);
int			zc_cstrlist_append_format(zcCStrList*, const char *format, ...);

#define		zc_cstrlist_len(s) s->n

#endif
