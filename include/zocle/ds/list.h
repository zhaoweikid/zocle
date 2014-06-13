#ifndef ZOCLE_DS_LIST_H
#define ZOCLE_DS_LIST_H

#include <stdio.h>
#include <zocle/base/defines.h>
#include <zocle/ds/listhead.h>
#include <zocle/base/type.h>
    
typedef struct zc_listnode_t
{
    struct zc_listhead_t head;
    void   *data;
}zcListNode;

typedef struct zc_list_t
{
	ZC_OBJECT_HEAD
    struct zc_listhead_t head;
    int    size;

    zcFuncDel del;
    zcFuncCmp cmp;
}zcList;

zcList* zc_list_new();
int     zc_list_new2(zcList**);
void    zc_list_delete(void *list);
void    zc_list_delete4(void *list, void *x, const char *, int);
void    zc_list_delete_node(zcList *list, zcListNode *node);
void    zc_list_clear(zcList *list);
int     zc_list_append(zcList *list, void*);
int     zc_list_prepend(zcList *list, void*);
int     zc_list_insert(zcList *list, void *node, int pos);
void*	zc_list_pop(zcList *list, int pos, void* defv);
void*	zc_list_at(zcList *list, int pos, void *defv);
int     zc_list_index(zcList *list, void *);
int     zc_list_remove(zcList *list, void *);
int     zc_list_reverse(zcList *list);
void    zc_list_print(zcList *list);

struct zc_listhead_t*   zc_list_find_data(zcList *list, void *data, int *pos);

#define zc_list_delete_null(x)   do{zc_list_delete(x);x=NULL;}while(0)

#define zc_list_foreach(_list, _node) \
    zc_listhead_for_each_entry(_node,&_list->head,head)

#define zc_list_foreach_safe(list,item,tmp) \
    zc_listhead_for_each_entry_safe(item,tmp,&list->head,head)


#endif
