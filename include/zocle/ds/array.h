#ifndef ZOCLE_DS_ARRAY_H
#define ZOCLE_DS_ARRAY_H

#include <stdio.h>
#include <stdint.h>
#include <zocle/base/defines.h>
#include <zocle/base/type.h>

typedef struct zc_array_t
{
    // FIXME: 如果cap的长度大于0x80000000，那len就会溢出
	uint32_t cap;
	uint32_t len:31;
    // data数据是否保存在结构体尾
    // 0: 不保存在结构体尾, data数据另行分配
    // 1: 保存在结构体尾, data数据与结构体一起分配
	uint32_t data_tail:1;
	zcFuncDel del;
    //zcFuncCmp cmp;
	void	**data;
}zcArray;

zcArray*    zc_array_new(uint32_t);
zcArray*    zc_array_new_tail(uint32_t);
void		zc_array_delete(void*);
int			zc_array_init(zcArray *, uint32_t);
void		zc_array_destroy(void*);

void*		zc_array_get(zcArray*, uint32_t pos, void *defv);
void*		zc_array_at(zcArray*, uint32_t pos, void *defv);
int			zc_array_set(zcArray*, uint32_t pos, void *v);
int			zc_array_set_many(zcArray*, uint32_t len, void *v);
int			zc_array_append(zcArray*, void *v);
void*		zc_array_pop_back(zcArray*, void *defv);
int			zc_array_resize(zcArray*, uint32_t newsize);


#endif
