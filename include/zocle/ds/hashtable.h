#ifndef ZOCLE_DS_HASHTABLE_H
#define ZOCLE_DS_HASHTABLE_H

#include <stdio.h>
#include <zocle/ds/hashset.h>
#include <zocle/utils/funcs.h>

typedef int (*zcHashTableWalkFunc)(const char *key, void *value, void *userdata);

typedef struct zc_hashtable_node_t
{
    ZC_HASHSET_NODE_HEAD
    void    *value;
}zcHashTableNode;

//zcHashTableNode* zc_hashtable_node_new(char *key, int klen, void *value);
//void             zc_hashtable_node_delete(void *);

typedef struct zc_hashtable_t
{
	ZC_HASHSET_HEAD	
    zcFuncDel   valdel;
}zcHashTable;


zcHashTable* zc_hashtable_new(int size);
zcHashTable* zc_hashtable_new_full(int size, zcFuncHash hash, 
                                  zcFuncCmp cmp, zcFuncDel kdel,
                                  zcFuncDel vdel,zcFuncDel2 ndel);
int          zc_hashtable_new2(zcHashTable **, int size);
int          zc_hashtable_new2_full(zcHashTable **, int size,
                                  zcFuncHash hash, zcFuncCmp cmp,
                                  zcFuncDel kdel, zcFuncDel vdel, 
                                  zcFuncDel2 ndel);
void         zc_hashtable_delete(void *);
void         zc_hashtable_clear(zcHashTable *);
void         zc_hashtable_print(zcHashTable *);

//int           zc_hashtable_add_node(zcHashTable *, zcHashTableNode *);
int          zc_hashtable_add(zcHashTable *, char *key, int, void *value);
int          zc_hashtable_haskey(zcHashTable *, char *key, int);
int          zc_hashtable_rm(zcHashTable *, char *key, int);
int		     zc_hashtable_set(zcHashTable *, char *key, int, void *value);
void*	     zc_hashtable_get(zcHashTable *, char *key, int, void *defv);
int          zc_hashtable_resize(zcHashTable *, int newsize);


uint32_t     zc_hashtable_size(zcHashTable *);
uint32_t     zc_hashtable_len(zcHashTable *);
void		 zc_hashtable_walk(zcHashTable *, zcHashTableWalkFunc f, void *userdata);

#define      zc_hashtable_delete_null(ht)   do { zc_hashtable_delete(ht); ht = NULL; } while (0)

#define      zc_hashtable_foreach_start(ht,k,v) \
             {int _zc_ht_i; \
			 zcHashTableNode *_zc_ht_node;\
             for (_zc_ht_i = 0; _zc_ht_i < (int)ht->size; _zc_ht_i++) {\
                 _zc_ht_node = (zcHashTableNode*)ht->bunks[_zc_ht_i];\
			  	 for (;_zc_ht_node;_zc_ht_node = (zcHashTableNode*)_zc_ht_node->next) { \
				  	 k = _zc_ht_node->key;\
					 v = _zc_ht_node->value;

#define		 zc_hashtable_foreach_end \
                }\
             }}

#define		 zc_hashtable_node_foreach_start(ht,node)	\
             {int _zc_ht_i; \
             for (_zc_ht_i = 0; _zc_ht_i < (int)ht->size; _zc_ht_i++) {\
                 node = (zcHashTableNode*)ht->bunks[_zc_ht_i];\
			     for (;node;node = (zcHashTableNode*)node->next) {\

#define		 zc_hashtable_node_foreach_end	\
				}\
			 }}

#endif
