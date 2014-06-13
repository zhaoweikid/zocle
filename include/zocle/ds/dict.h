#ifndef ZOCLE_DS_DICT_H
#define ZOCLE_DS_DICT_H

#include <stdio.h>
#include <stdint.h>
#include <zocle/base/defines.h>
#include <zocle/base/object.h>

extern const char *ZC_KEY_DUMMY;
typedef int (*zcDictWalkFunc)(const char*, void*, void*);

typedef struct zc_dict_node_t
{
    uint32_t hash;
    char *key;
    void *value;
}zcDictNode;

zcDictNode* zc_dict_node_init(zcDictNode *, char *key, int keylen, void *value);
void	    zc_dict_node_destroy(void *ht, zcFuncDel keydel, zcFuncDel valdel);

typedef struct zc_dict_t
{
	ZC_OBJECT_HEAD
    char reduce;
    uint32_t size;
    uint32_t filled;
    uint32_t len;
    zcDictNode *table;

    zcFuncHash hash;
	zcFuncDel  keydel;
	zcFuncDel  valdel;
}zcDict;

zcDict* zc_dict_new(int minsize, char reduce);
zcDict* zc_dict_new_full(int minsize, char reduce, zcFuncDel keydel, zcFuncDel valdel);
void    zc_dict_delete(void *h);
void    zc_dict_clear(zcDict *ht);
int     zc_dict_add(zcDict *ht, const char *key, int keylen, void *value);
int     zc_dict_set(zcDict *ht, const char *key, int keylen, void *value);
void*   zc_dict_get(zcDict *ht, const char *key, int keylen, void *defv);
int     zc_dict_rm(zcDict *ht, const char *key, int keylen);
int     zc_dict_haskey(zcDict *ht, const char *key, int keylen);
int     zc_dict_resize(zcDict *ht, uint32_t minsize);
int     zc_dict_walk(zcDict *ht, zcDictWalkFunc walkfunc, void *userdata);
void    zc_dict_print(zcDict *ht);

#define zc_dict_add_str(ht,key,value) zc_dict_add(ht,key,0,value)
#define zc_dict_set_str(ht,key,value) zc_dict_set(ht,key,0,value)
#define zc_dict_get_str(ht,key,defv)  zc_dict_get(ht,key,0,defv)
#define zc_dict_rm_str(ht,key)		   zc_dict_rm(ht,key,0)
#define zc_dict_haskey_str(ht,key)	   zc_dict_haskey(ht,key,0)

#define zc_dict_new_obj(sz) zc_dict_new_full(sz,0,zc_free_func,zc_obj_delete)

#define zc_dict_foreach_start(ht,k,v) \
    {int _zc_ht_n = ht->len;\
    for (zcDictNode *node=ht->table; _zc_ht_n>0; node++) {\
        if (node->key != NULL && node->key != ZC_KEY_DUMMY) {\
			k = node->key;\
			v = node->value;

#define zc_dict_foreach_end  _zc_ht_n--; } }}

#define zc_dict_node_foreach(ht,node) \
    for (node=ht->table; ht->len>0; node++)


#endif
