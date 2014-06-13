#ifndef ZOCLE_DS_ARRAY_H
#define ZOCLE_DS_ARRAY_H

#include <stdio.h>
#include <stdint.h>
#include <zocle/base/defines.h>
#include <zocle/base/type.h>

typedef struct zc_array_t
{
	uint32_t capacity;
	uint32_t len;
	void	**data;

	zcFuncDel del;
    zcFuncCmp cmp;
}zcArray;

zcArray*    zc_array_new(uint32_t);
void		zc_array_delete(void*);
int			zc_array_init(zcArray *, uint32_t);
void		zc_array_destroy(void*);

void*		zc_array_get(zcArray*, uint32_t pos, void *defv);
void*		zc_array_at(zcArray*, uint32_t pos, void *defv);
int			zc_array_set(zcArray*, uint32_t pos, void *v);
int			zc_array_append(zcArray*, void *v);
void*		zc_array_pop_back(zcArray*, void *defv);
int			zc_array_resize(zcArray*, uint32_t newsize);


#endif
