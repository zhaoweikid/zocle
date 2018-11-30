#ifndef ZOCLE_DS_HASHSET_H
#define ZOCLE_DS_HASHSET_H

#include <stdio.h>
#include <stdint.h>
#include <zocle/utils/funcs.h>
#include <zocle/base/defines.h>

#define ZC_HASHSET_NODE_HEAD \
    struct zc_hashset_node_t *next; \
	char *key;

typedef struct zc_hashset_node_t
{
    ZC_HASHSET_NODE_HEAD
}zcHashSetNode;


//zcHashSetNode* zc_hashset_node_new(char *key, int);
//void           zc_hashset_node_delete(void*);

#define ZC_HASHSET_HEAD \
    uint32_t    size;\
    uint32_t    len;\
    void		**bunks;\
    zcFuncHash	hash;\
    zcFuncCmp   cmp;\
	zcFuncDel	keydel;\
    zcFuncDel2  nodedel;

typedef struct zc_hashset_t
{
    ZC_HASHSET_HEAD
}zcHashSet;

typedef int (*zcHashSetWalkFunc)(zcHashSetNode *, void *userdata);

zcHashSet* zc_hashset_new(int size);
zcHashSet* zc_hashset_new_full(int size, zcFuncHash hash, zcFuncCmp cmp,
								 zcFuncDel keydel, zcFuncDel2 nodedel);
int        zc_hashset_new2(zcHashSet **, int size);
int        zc_hashset_new2_full(zcHashSet **, int size,
                                zcFuncHash hash, zcFuncCmp cmp,
								zcFuncDel keydel, zcFuncDel2 nodedel);
void       zc_hashset_delete(void *);
void       zc_hashset_clear(zcHashSet *);
void       zc_hashset_print(zcHashSet *);
int        zc_hashset_add_node(zcHashSet *, zcHashSetNode *);
int        zc_hashset_add(zcHashSet *, char *key, int);
int        zc_hashset_rm(zcHashSet *, char *key, int);
int        zc_hashset_haskey(zcHashSet *, char *key, int);
int        zc_hashset_resize(zcHashSet *, int newsize);
uint32_t   zc_hashset_size(zcHashSet *);
uint32_t   zc_hashset_len(zcHashSet *);

zcHashSetNode*  zc_hashset_lookup_key(zcHashSet *h, char *key, int keylen);

#define    zc_hashset_delete_null(x)    \
           do{zc_hashset_delete(x,NULL);x=NULL;}while(0)

#define    zc_hashset_foreach_start(hset,hnode)   \
           for (int _zc_hs_i = 0; _zc_hs_i < hset->size; _zc_hs_i++) {\
               hnode = (zcHashSetNode*)hset->bunks[_zc_hs_i];\
			   for (;hnode;hnode = (zcHashSetNode*)hnode->next) {

#define    zc_hashset_foreach_end \
               }}


#endif
