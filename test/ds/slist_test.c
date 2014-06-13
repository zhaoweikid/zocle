#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <zocle/zocle.h>
#include <stddef.h>

typedef struct 
{
    ZC_SLIST_HEAD
    int v;
}MyListNode;

int mylistnode_cmp(void *a, void *b, int len)
{
    int a1 = *(int*)a;
    int b1 = (long)b;

    //ZCINFO("slist cmp: %d, %d\n", a1, b1);
    if (a1 == b1)
        return 0;
    if (a1 > b1);
        return 1;
    return -1;
}


int test_slisthead()
{
    int i;
    int count = 1000;
    MyListNode *root = NULL, *node;
    //, *prev = NULL;
   
    for (i = 0; i < count; i++) {
        node = (MyListNode*)zc_slisthead_new(MyListNode); 
        node->v = i;
        zc_check(node);
        root = (MyListNode*)zc_slisthead_append((zcSListHead*)root, (zcSListHead*)node);
    }

    node = root;
    i = 0;
    while (node) {
        assert(node->v == i);
        ZCINFO("1 node: %p, %d, %p\n", node, node->v, node->next);
        node = (MyListNode*)node->next;
        i++;
    }
    ZCINFO("i:%d\n", i); 


    zcSListHead *myroot = (zcSListHead*)root;
    zc_slisthead_foreach_safe(root, node) {
        ZCINFO("1 foreach: %p, %d, %p, tmp:%p, root:%p\n", node, node->v, node->next, _zc_slh_tmp, root);
    }
    
    zcSListHead *broot = (zcSListHead*)root;
    zcSListHead *bprev;

    node = (MyListNode*)zc_slisthead_pos(broot , 10, &bprev);
    assert(node != NULL);
    assert(node->v == 10);
    assert(bprev->next == (zcSListHead*)node);
    
    zc_check(node);
    i = zc_slisthead_find(broot, (void*)300, mylistnode_cmp, (zcSListHead**)&node, NULL);
    assert(node != NULL);
    assert(node->v == 300);

    zc_check(node);
    i = zc_slisthead_index(broot, (zcSListHead*)node);
    assert(i == 300);

    zc_check(node);
    root = (MyListNode*)zc_slisthead_unlink(broot, (zcSListHead*)node, &bprev);
    assert(root != NULL);
    zc_check(node);
    zc_slisthead_delete(node);

    i = zc_slisthead_find(broot, (void*)300, mylistnode_cmp, (zcSListHead**)&node, NULL);
    assert(node == NULL);
 
    zc_slisthead_foreach_self(myroot) {
        MyListNode *nn = (MyListNode*)myroot;
        //ZCINFO("foreach self: %p, %d\n", myroot, nn->v);
        assert(nn != NULL);
    }
    root = (MyListNode*)zc_slisthead_reverse(broot);
    assert(root->v == 999);

    return 0;
}

typedef struct
{
    ZC_SLIST_HEAD
    char    str[64];
}MyStrNode;

int test_slisthead_str()
{
    int  i;
    char buf[64];
    MyStrNode *root = NULL, *node, *prev = NULL;
   
    for (i = 0; i < 1000; i++) {
        node = (MyStrNode*)zc_slisthead_new(MyStrNode); 
        sprintf(node->str, "x%d", i);

        if (prev) {
            prev->next = (zcSListHead*)node;
        }

        if (root == NULL) {
            root = node;
        }
        prev = node;
    }

    node = root;
    i = 0;
    while (node) {
        sprintf(buf, "x%d", i);
        assert(strcmp(buf, node->str) == 0);
        node = (MyStrNode*)node->next;
        i++;
    }
    
    sprintf(buf, "x%d", 10);
    node = (MyStrNode*)zc_slisthead_pos((zcSListHead*)root , 10, (zcSListHead**)&prev);
    assert(node != NULL);
    assert(strcmp(buf, node->str) == 0);
    assert(prev->next == (zcSListHead*)node);
    
    sprintf(buf, "x%d", 10);
    i = zc_slisthead_find((zcSListHead*)root, (void*)buf, zc_cmp_str, (zcSListHead**)&node, NULL);
    assert(node != NULL);
    assert(strcmp(buf, node->str) == 0);

    i = zc_slisthead_index((zcSListHead*)root, (zcSListHead*)node);
    assert(i == 10);

    root = (MyStrNode*)zc_slisthead_unlink((zcSListHead*)root, (zcSListHead*)node, (zcSListHead**)&prev);
    assert(root != NULL);
    zc_slisthead_delete(node);

    i = zc_slisthead_find((zcSListHead*)root, (void*)buf, zc_cmp_int, (zcSListHead**)&node, NULL);
    assert(node == NULL);
    
    sprintf(buf, "x%d", 999);
    root = (MyStrNode*)zc_slisthead_reverse((zcSListHead*)root);
    assert(strcmp(root->str, buf) == 0);


    return 0;
}

typedef struct
{
    ZC_SLIST_HEAD
    char    *str;
}MyPStrNode;

int test_slisthead_pstr()
{
    int  i;
    char buf[64];
    MyPStrNode *root = NULL, *node, *prev = NULL;
   
    for (i = 0; i < 1000; i++) {
        node = (MyPStrNode*)zc_slisthead_new(MyPStrNode); 
        node->str = zc_malloc(100);
        sprintf(node->str, "x%d", i);

        if (prev) {
            prev->next = (zcSListHead*)node;
        }

        if (root == NULL) {
            root = node;
        }
        prev = node;
    }

    node = root;
    i = 0;
    while (node) {
        sprintf(buf, "x%d", i);
        assert(strcmp(buf, node->str) == 0);
        node = (MyPStrNode*)node->next;
        i++;
    }
    
    sprintf(buf, "x%d", 10);
    node = (MyPStrNode*)zc_slisthead_pos((zcSListHead*)root , 10, (zcSListHead**)&prev);
    assert(node != NULL);
    assert(strcmp(buf, node->str) == 0);
    assert(prev->next == (zcSListHead*)node);
    
    sprintf(buf, "x%d", 10);
    i = zc_slisthead_find((zcSListHead*)root, (void*)buf, zc_cmp_pstr, (zcSListHead**)&node, NULL);
    assert(node != NULL);
    assert(strcmp(buf, node->str) == 0);

    i = zc_slisthead_index((zcSListHead*)root, (zcSListHead*)node);
    assert(i == 10);

    root = (MyPStrNode*)zc_slisthead_unlink((zcSListHead*)root, (zcSListHead*)node, (zcSListHead**)&prev);
    assert(root != NULL);
    zc_slisthead_delete(node);

    i = zc_slisthead_find((zcSListHead*)root, (void*)buf, zc_cmp_pint, (zcSListHead**)&node, NULL);
    assert(node == NULL);
    
    sprintf(buf, "x%d", 999);
    root = (MyPStrNode*)zc_slisthead_reverse((zcSListHead*)root);
    assert(strcmp(root->str, buf) == 0);

    return 0;
}

int test_slist()
{
    zcSList     *list;

    list = zc_slist_new();
    assert(list != NULL);

    long i;
    for (i = 0; i < 100; i++) {
        zc_slist_append(list, (void*)i); 
    }
    
    assert(list->size == 100);

    long data;
    i = 0;
    zc_slist_foreach(list,data) {
        ZCINFO("data: %ld\n", data);
        assert(data == i);
        i++;
    }

    for (i = 0; i < 100; i++) {
        data = (long)zc_slist_at(list, i, NULL);
        assert(data == i);
    }

    for (i = 0; i < 100; i++) {
        data = (long)zc_slist_pop_front(list);
        assert(data == i);
    }

    for (i = 0; i < 100; i++) {
        assert(zc_slist_prepend(list, (void*)i) == ZC_OK);
    }
    
    for (i = 0; i < 100; i++) {
        data = (long)zc_slist_pop_back(list);
        assert(data == i);
    }

    for (i = 0; i < 100; i++) {
        assert(zc_slist_append(list, (void*)i) == ZC_OK);
    }
 
    long v = 1000;
    assert(zc_slist_append(list, (void*)v) == 0);
    assert(v == (long)zc_slist_last(list));
    assert(list->size == 101);

    zc_check(list);
    //assert(zc_slist_unlink(list, node) == ZC_OK);
    //zc_check(list);

    //zc_slisthead_delete(node);
    //zc_slisthead_delete_null(node);
    //assert(node == NULL);

    zcSListNode *node;
    zc_slist_node_foreach(list, node) {
        ZCINFO("node:%p, %ld\n", node, (long)node->data);
    }

    assert(zc_slist_set_cmp(list, zc_cmp_int) == ZC_OK);
    v = 1;
    ZCINFO("1 size: %d\n", list->size);
    int oldsize = list->size;
    zc_slist_unlink_data(list, (void*)v);
    //node = zc_slist_unlink_data(list, (void*)v);
    //assert(node != NULL);
    //assert(node->data == (void*)1);
    ZCINFO("2 size: %d\n", list->size);
    assert(list->size == oldsize-1);
    
    i = 10; 
    data = (long)zc_slist_get(list, i);
    //assert(node != NULL);
    //assert(node->data == (void*)(i+1));
    assert(list->size == oldsize-2);

    long x;
    zc_slist_foreach(list, x) {
        ZCINFO("v:%ld\n", x);
    }
    
    ZCINFO("insert at:%ld value:%ld\n", i, v);
    assert(zc_slist_insert(list, (void*)v, i) == ZC_OK);

    /*zc_slist_data_foreach(list, x) {
        ZCINFO("v:%ld\n", x);
    }*/

    assert(list->size == oldsize-1); 
    ZCINFO("index: %d\n", zc_slist_index(list, (void*)v));
    assert(zc_slist_index(list, (void*)v) == i);
    
    long v2 = (long)zc_slist_at(list, i, 0);
    assert(v2 == v);

    v = 0;
    ZCINFO("remove pos:%ld\n", v);
    assert(zc_slist_remove_pos(list, 0) == ZC_OK);
    assert(list->size == oldsize-2);
    assert(zc_slist_find(list, (void*)v) == NULL);

    v = 2;
    ZCINFO("remove data:%ld\n", v);
    assert(zc_slist_remove(list, (void*)v) == ZC_OK);

    /*zc_slist_data_foreach(list, x) {
        ZCINFO("v:%ld\n", x);
    }*/

    assert(list->size == oldsize-3);
    assert(zc_slist_find(list, (void*)v) == NULL);

    v = 90;
    zcSListNode *node2 = zc_slist_find(list, (void*)v);
    assert(node2 != NULL);
    assert(zc_slist_remove_node(list, node2) == ZC_OK);
    assert(zc_slist_find(list, (void*)v) == NULL);
    assert(zc_slist_length(list) == list->size);
   
    int oldsize2 = list->size;
    zc_slist_reverse(list);
    assert(oldsize2 == list->size); 
    v = (long)zc_slist_at(list, 0, 0);
    assert(v == 1000);
    v = (long)zc_slist_at(list, -1, 0);
    assert(v == 3);

    zc_slist_delete(list);

    return 0;
}

int test_slist2()
{
    zcSList     *list;

    list = zc_slist_new();
    assert(list != NULL);
   
    int n;
    for (n=0; n<3; n++) {
        long i;
        for (i = 0; i < 100; i++) {
            if (n == 0) {
                assert(zc_slist_insert(list, (void*)i, -1) == ZC_OK);
            }else if (n == 1) {
                assert(zc_slist_push_front(list, (void*)i) == ZC_OK);
            }else {
                assert(zc_slist_push_back(list, (void*)i) == ZC_OK);
            }
        }

        assert(list->size == 100);

        i = 0;
        long val;
        zc_slist_foreach(list, val) {
            //ZCINFO("val:%ld i:%ld\n", val, i);
            if (n == 0) {
                assert(val == i);
            }else if (n == 1) {
                assert(val == 99-i);
            }else{
                assert(val == i);
            }
            i++; 
        }
        zc_slist_clear(list); 
    }


    zc_slist_delete(list);

    return 0;
}    
 

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW); 
    zc_log_new("stdout", ZC_LOG_ALL);

    ZCINFO("==== test mynode ====\n");
    test_slisthead();
    ZCINFO("==== test strnode ====\n");
    test_slisthead_str();
    ZCINFO("==== test pstrnode ====\n");
    test_slisthead_pstr();
    ZCINFO("==== test slist ====\n");
    test_slist();
    test_slist2();

    return 0;
}
