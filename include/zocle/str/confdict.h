#ifndef ZOCLE_STR_CONFDICT_H
#define ZOCLE_STR_CONFDICT_H

#include <zocle/ds/dict.h>
#include <zocle/ds/list.h>
#include <stdint.h>
#include <limits.h>

typedef struct zc_confdict_t
{
	zcDict	*groups;
	zcDict  *group_default;
	char    filename[PATH_MAX];
	char	kvsp;
	char	vvsp;
}zcConfDict;

zcConfDict*	zc_confdict_new(const char *filename);
void		zc_confdict_delete(void*);
int         zc_confdict_parse(zcConfDict *cd);
void*		zc_confdict_get(zcConfDict*, const char *group, const char *key);
char*		zc_confdict_get_str(zcConfDict*, 
                    const char *group, 
                    const char *key, char *defv);
int			zc_confdict_get_int(zcConfDict*, 
                    const char *group, 
                    const char *key, int defv);
double		zc_confdict_get_double(zcConfDict*, 
                    const char *group, 
                    const char *key, double defv);
int64_t		zc_confdict_get_int64(zcConfDict*, 
                    const char *group, 
                    const char *key, int64_t defv);
zcList*		zc_confdict_get_list(zcConfDict*, 
                    const char *group, const char *key);
int			zc_confdict_get_array_int(zcConfDict *cd, 
                    const char *group, 
                    const char *key, int *value, int vlen);
int			zc_confdict_get_array_int64(zcConfDict *cd, 
                    const char *group, 
                    const char *key, 
                    int64_t *value, int vlen);
int			zc_confdict_get_array_double(zcConfDict *cd, 
                    const char *group, 
                    const char *key, 
                    double *value, int vlen);
void        zc_confdict_print(zcConfDict *cd);

#endif
