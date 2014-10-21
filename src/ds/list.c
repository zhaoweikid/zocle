#include <zocle/ds/list.h>
#include <zocle/mem/alloc.h>
#include <zocle/utils/funcs.h>
#include <zocle/log/logfile.h>
#include <string.h>

static zcListNode* 
zc_listnode_new_data(void *data)
{
    zcListNode  *node = zc_malloct(zcListNode);
    //zc_listhead_init(&node->list);
    node->data = data;

    return node;
}

zcList*     
zc_list_new()
{
    zcList  *list;
    if (zc_list_new2(&list) != ZC_OK)
        return NULL;
    return list;
}

int 
zc_list_new2(zcList **list)
{
    zcList  *newlist;
    *list = NULL;

    newlist = (zcList*)zc_malloc(sizeof(zcList));
    memset(newlist, 0, sizeof(zcList));
    newlist->__type = ZC_LIST;
    newlist->cmp = zc_cmp_simple;
    zc_listhead_init(&newlist->head);
    *list = newlist;
    
    return ZC_OK;
}

void
zc_list_delete(void *list)
{
    zcList  *mylist = (zcList*)list;
    zc_list_clear(mylist);
    zc_free(mylist);
}

void
zc_list_delete_node(zcList *list, zcListNode *node)
{
    if (list->del)
        list->del(node->data);
    zc_free(node);
}

void
zc_list_clear(zcList *list)
{
    struct zc_listhead_t *root;
    root = &list->head;
    zcListNode *cur, *tmp;
    zc_listhead_for_each_entry_safe(cur, tmp, root, head) {
        //zcListNode *node = zc_listhead_entry(cur, zcListNode*, list);
        if (list->del) {
            list->del(cur->data);
        }
        zc_free(cur);
    }
    list->size = 0;
    zc_listhead_init(&list->head);
}

int 
zc_list_append(zcList *list, void *data)
{
    zcListNode *node = zc_listnode_new_data(data);
    zc_listhead_add_tail(&node->head, &list->head);
    list->size++;
    return ZC_OK;
}

int 
zc_list_prepend(zcList *list, void *data)
{
    zcListNode *node = zc_listnode_new_data(data);
    zc_listhead_add(&node->head, &list->head);
    list->size++;
    return ZC_OK;
}

static struct zc_listhead_t*
zc_list_find_pos(zcList *list, int pos)
{
    /*if (list->size == 0)
        return NULL;*/
   
    int i = 0;
    struct zc_listhead_t *cur = NULL;
    if (pos >= 0) {
        zc_listhead_for_each(cur, &list->head) {
            if (i == pos)
                break;
            i++;
        }
    }else{
        i--;
        zc_listhead_for_each_prev(cur, &list->head) {
            if (i == pos)
                break;
            i--;
        }
    }
    return cur;
}


void* 
zc_list_pop(zcList *list, int pos, void *defv)
{
    struct zc_listhead_t *cur = zc_list_find_pos(list, pos);
    if (NULL == cur)
        return defv;

    zcListNode  *node = zc_listhead_entry(cur, zcListNode, head);
    zc_listhead_del_entry(cur);
    list->size--;

    void *data = node->data;
    zc_free(node); 
    return data;
}


int         
zc_list_insert(zcList *list, void *data, int pos)
{
    struct zc_listhead_t *cur = zc_list_find_pos(list, pos);
    if (NULL == cur)
        return ZC_ERR;
   
    zcListNode *node = zc_listnode_new_data(data);
    if (pos >= 0) {
        zc_listhead_add_bt(&node->head, cur->prev, cur);
    }else{
        zc_listhead_add_bt(&node->head, cur, cur->next);
    }
    list->size++;
    return ZC_OK;
}

void* 
zc_list_at(zcList *list, int pos, void *defv)
{
    struct zc_listhead_t *cur = zc_list_find_pos(list, pos);
    if (NULL == cur)
        return defv;
    zcListNode *node = zc_listhead_entry(cur, zcListNode, head);
    return node->data;
}

struct zc_listhead_t*
zc_list_find_data(zcList *list, void *data, int *pos)
{
    int i = 0;
    struct zc_listhead_t *cur = NULL;
    zcListNode *node = NULL;
    zc_listhead_for_each(cur, &list->head) {
        node = zc_listhead_entry(cur, zcListNode, head);
        if (list->cmp(node->data, data, 0) == 0) {
            *pos = i;
            return cur;
        }
        i++;
    }
    return NULL;
}


int 
zc_list_index(zcList *list, void *data)
{
    /*if (NULL == data)
        return ZC_ERR_NULL;*/
    int i = 0; 
    struct zc_listhead_t *cur = zc_list_find_data(list, data, &i);
    if (NULL == cur)
        return ZC_ERR_NOT_FOUND;
    return i;
}

int 
zc_list_remove(zcList *list, void *data)
{
    if (NULL == data)
        return ZC_ERR_NULL;
    int i = 0; 
    struct zc_listhead_t *cur = zc_list_find_data(list, data, &i);
    if (NULL == cur)
        return ZC_ERR_NOT_FOUND;
    zcListNode *node = zc_listhead_entry(cur, zcListNode, head);
    if (list->del)
        list->del(node->data);
    zc_listhead_del_entry(cur);
    list->size--;
    zc_free(node);
    return ZC_OK;
}

int 
zc_list_reverse(zcList *list)
{
    struct zc_listhead_t newroot;
    zc_listhead_init(&newroot);

    struct zc_listhead_t *cursor, *tmp;
    zc_listhead_for_each_safe(cursor, tmp, &list->head) {
        zc_listhead_del_entry(cursor);
        zc_listhead_add(cursor, &newroot);
    }
    zc_listhead_replace(&newroot, &list->head);
    return ZC_OK;
}

void
zc_list_print(zcList *list)
{
    if (NULL == list) {
        ZCINFO("list is NULL\n");
        return;
    }

    zcListNode *node;
    ZCINFO("list size: %d\n", list->size);
    int i=0;
    zc_listhead_for_each_entry(node, &list->head, head) {
        ZCINFO("%d\t%p | %p(%ld)\n", i, node, node->data, (long)node->data);
        //node = (zcListNode*)node->next;
        i++;
    }
}





