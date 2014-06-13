#ifdef ZOCLE_WITH_PCRE

#ifndef ZOCLE_STRING_REGEXP_H
#define ZOCLE_STRING_REGEXP_H

#include <stdio.h>
#include <pcre.h>
#include <zocle/ds/list.h>

typedef struct zc_re_match_t
{
	int		start;
	int		end;
	char	*result;
	zcList  *groups;
	int		*vec;
}zcRegMatch;

ZOCLE_API zcRegMatch*	zc_regmatch_new();
ZOCLE_API void			zc_regmatch_delete(void *);
ZOCLE_API int			zc_regmatch_group_num(zcRegMatch *);
ZOCLE_API char*			zc_regmatch_group_at(zcRegMatch *, int);
ZOCLE_API char*			zc_regmatch_result(zcRegMatch *);
ZOCLE_API void			zc_regmatch_range(zcRegMatch *, int *start, int *end);


typedef struct zc_re_t
{
    pcre        *re;
    pcre_extra  *extra;
    int         vecsize;
}zcRegExp;

ZOCLE_API   zcRegExp*   zc_regexp_new(char *res, int options);
ZOCLE_API   void        zc_regexp_delete(void *);

ZOCLE_API	int			zc_regexp_init(zcRegExp*, char *res, int options);
ZOCLE_API   void        zc_regexp_destroy(void *);

ZOCLE_API   zcRegMatch* zc_regexp_search(zcRegExp*, const char *s, int slen);
ZOCLE_API   zcList*	    zc_regexp_findall(zcRegExp*, const char *s, int slen);
ZOCLE_API   zcList*	    zc_regexp_split(zcRegExp*, const char *s, int slen);

#define     zc_regexp_delete_null(x) do{zc_regexp_delete(x);x=NULL;}while(0)

#endif
#endif
