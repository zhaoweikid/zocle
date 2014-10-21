#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <zocle/zocle.h>

typedef struct 
{
    struct zc_listhead_t list;
    int v;
}MyListNode;

int mylistnode_cmp(void *a, void *b, int len)
{
    int a1 = *(int*)a;
    int b1 = (int)(long)b;
    
    //ZCINFO("cmp: %d, %d\n", a1, b1);
    if (a1 == b1)
        return 0;
    if (a1 > b1)
        return 1;
    return -1;
}


int test_listnode()
{
    int i;
    struct zc_listhead_t root;
    zc_listhead_init(&root);

    MyListNode *node;
   
    for (i = 0; i < 100; i++) {
        node = (MyListNode*)zc_malloct(MyListNode); 
        node->v = i;
        zc_listhead_add_tail(&node->list, &root);
        //ZCINFO("node:%p prev:%p next:%p, %d\n", 
        //        node, node->list.prev, node->list.next, node->v);
    }

    i = 0;
    struct zc_listhead_t *tmp, *cursor;
    zc_listhead_for_each(cursor, &root) {
        node = zc_listhead_entry(cursor, MyListNode, list);
        //ZCINFO("1 v: %p %d next:%p\n", node, node->v, node->list.next);
        assert(node->v == i);
        i++;
    }
    i = 0;
    zc_listhead_for_each_safe(cursor, tmp, &root) { 
        node = zc_listhead_entry(cursor, MyListNode, list);
        //ZCINFO("2 v: %p %d next:%p\n", node, node->v, node->list.next);
        assert(node->v == i);
        i++;
    }
    
    return 0;
}


int test_list()
{
    zcList     *list;
    int oldsize;

    list = zc_list_new();
    assert(list != NULL);
    list->cmp = zc_cmp_int;

    long i;
    for (i = 0; i < 100; i++) {
        assert(zc_list_append(list, (void*)i) == ZC_OK);
    }
    
    //ZCINFO("list size:%d\n", list->size);
    assert(list->size == 100);

    long data;
    i = 0;
    zcListNode *hnode;
    struct zc_listhead_t *cursor;
    zc_listhead_for_each(cursor, &list->head) {
        hnode = zc_listhead_entry(cursor, zcListNode, head);
        //ZCINFO("foreach %ld, head:%p data:%ld\n", i, &list->head, (long)hnode->data);
        assert((long)hnode->data == i);
        i++;
    }

    i = 0;
    zc_list_foreach(list, hnode) {
        //ZCINFO("foreach %ld, head:%p data:%ld\n", i, &list->head, (long)hnode->data);
        assert((long)hnode->data == i);
        i++;
    }

    //ZCINFO("count:%ld first:%p\n", i, &list->head);

    for (i = 0; i < 100; i++) {
        data = (long)zc_list_at(list, i, (void*)-1);
        assert(data != -1);
        assert(data == i);
    }

    for (i = 0; i < 100; i++) {
        data = (long)zc_list_pop(list, 0, NULL);
        assert(data == i);
    }
    // empty
    for (i = 0; i < 100; i++) {
        assert(zc_list_prepend(list, (void*)i) == ZC_OK);
    }

    i = 100; 
    zc_list_foreach(list, hnode) {
        i--;
        //ZCINFO("data:%ld\n", (long)hnode->data);
        assert((long)hnode->data == i);
    }
    
    for (i = 0; i < 100; i++) {
        data = (long)zc_list_pop(list, -1, NULL);
        assert(data == i);
    }

    // empty

    for (i = 0; i < 100; i++) {
        assert(zc_list_append(list, (void*)i) == ZC_OK);
    }
   
    long v = 1000;
    assert(zc_list_append(list, (void*)v) == 0);
    assert(list->size == 101);

    zc_list_remove(list, (void*)v); // remove 1000

    zc_list_foreach(list, hnode) {
        assert((long)hnode->data != v);
    }

    // 100 
    v = 1;
    oldsize = list->size;
    i = zc_list_remove(list, (void*)v); // remove 1
    
    zc_list_foreach(list, hnode) {
        assert((long)hnode->data != v);
    }

    // 99
    //ZCINFO("size: %d\n", list->size);
    assert(list->size == oldsize-1);
    
    i = 10; 
    oldsize = list->size;
    data = (long)zc_list_pop(list, i, (void*)-1); // remove 11
    assert(data == i+1);
    assert(list->size == oldsize-1);
    // 98
    
    oldsize = list->size;
    assert(zc_list_insert(list, (void*)data, i) == ZC_OK); // insert 11 at 10
    assert(list->size == oldsize+1); 
    assert(zc_list_index(list, (void*)data) == i);


    // 99 
    v = 0;
    long vx[3] = {0, 2, 90};
    int thesize = list->size; 
    for (i=0; i<3; i++) {
        //zc_list_print(list);
        //ZCINFO("find %ld, size:%d, index:%d\n", vx[i], list->size, zc_list_index(list, (void*)vx[i]));
        assert(zc_list_index(list, (void*)vx[i]) >= 0);
        if (i == 0) {
            assert((long)zc_list_pop(list, 0, NULL) == 0);
            assert(zc_list_index(list, (void*)vx[i]) == ZC_ERR_NOT_FOUND);
            assert(list->size == thesize-i-1);
        }else if (i == 1) {
            assert(zc_list_remove(list, (void*)vx[i]) == ZC_OK);
            assert(zc_list_index(list, (void*)vx[i]) == ZC_ERR_NOT_FOUND);
            assert(list->size == thesize-i-1);
        }else{
            i = zc_list_index(list, (void*)vx[i]);
            assert(i >= 0);
        }
    }

    oldsize = list->size;
    zc_list_reverse(list);

    assert(oldsize == list->size); 
    data = (long)zc_list_at(list, 0, (void*)-1);
    //ZCINFO("first:%ld", data);
    assert(data == 99);

    //zc_list_print(list);
    data = (long)zc_list_at(list, -1, (void*)-1);
    //ZCINFO("at -1: %ld\n", data);
    assert(data == 3);

    zc_list_delete(list);


    // string
    list = zc_list_new();
    list->cmp  = zc_cmp_str;

    char buf[64];
    for (i = 0; i < 100; i++) {
        sprintf(buf, "x%ld", i);
        assert(zc_list_append(list, zc_strdup(buf,0)) == ZC_OK);
    }
    //ZCINFO("buf: %p, %p, %s\n", &buf, buf, buf); 
    //char *b = buf;
    assert(zc_list_index(list, buf) >= 0);
    
    zc_list_delete(list);

    return 0;
}

int test_list2()
{
    zcList     *list;

    list = zc_list_new();
    assert(list != NULL);
    int ret;
    long i;
    for (i = 0; i < 10; i++) {
        ret = zc_list_insert(list, (void*)i, -1);
        assert(ret == ZC_OK);
        zc_check(list->head.prev);
    }

    assert(list->size == 10);
 
    for (i = 0; i < 10; i++) {
        ret = zc_list_insert(list, (void*)i, -2);
        assert(ret == ZC_OK);
    }

   
    i = 1;
    zcListNode *node;
    zc_list_foreach(list, node) {
        if (i == 10) {
            assert((long)node->data == 0);
        }
        i++; 
    }

    return 0;
}    
 
int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW); 
    zc_log_new("stdout", ZC_LOG_ALL);

    ZCINFO("==== test mynode ====\n");
    test_listnode();
    ZCINFO("==== test list ====\n");
    test_list();
    ZCINFO("==== test list2 ====\n");
    test_list2();

    return 0;
}
