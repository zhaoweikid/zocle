#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <zocle/zocle.h>

typedef struct 
{
    ZC_LIST_HEAD
    int v;
}MyListNode;

int mylistnode_cmp(void *a, void *b, int len)
{
    int a1 = *(int*)a;
    int b1 = (int)(long)b;
    
    //ZCINFO("cmp: %d, %d\n", a1, b1);
    if (a1 == b1)
        return 0;
    if (a1 > b1);
        return 1;
    return -1;
}


int test_listnode()
{
    int i;
    MyListNode *root = NULL, *node;
   
    for (i = 0; i < 1000; i++) {
        node = (MyListNode*)zc_listhead_new(MyListNode); 
        node->v = i;
        if (root == NULL) {
            ZCINFO("root NULL prev:NULL next:NULL, node %p prev:%p next:%p, %d\n", 
                    node, node->prev, node->next, node->v);
            root = node;
        } else {
            //zc_listhead_add_head(node, root);
            zc_listhead_add_tail((zcListHead*)node, (zcListHead*)root);
            ZCINFO("root %p prev:%p next:%p, node %p prev:%p next:%p, %d\n", 
                    root, root->prev, root->next, node, node->prev, node->next, node->v);
            //root = node;
        }
    }
    ZCINFO("root:%p\n", root);
    zc_listhead_foreach_safe(root, node) {
        ZCINFO("v: %p %d next:%p\n", node, node->v, node->next);
    }
    
    node = root;
    i = 0;
    zc_listhead_foreach_self(node) {
        assert(node->v == i);
        i++;
    }
    
    node = (MyListNode*)zc_listhead_pos((zcListHead*)root , 10);
    assert(node != NULL);
    //ZCINFO("node->v: %d\n", node->v);
    assert(node->v == 10);
    
    i = zc_listhead_find((zcListHead*)root, (void*)300, mylistnode_cmp, (zcListHead**)&node);
    assert(i != ZC_ERR_NOT_FOUND);
    ZCINFO("i:%d node->v: %d\n", i, node->v);
    assert(node->v == 300);

    i = zc_listhead_index((zcListHead*)root, (zcListHead*)node);
    assert(i == 300);

    zc_check(node);
    root = (MyListNode*)zc_listhead_unlink((zcListHead*)root, (zcListHead*)node);
    assert(root != NULL);
    zc_check(node);
    zc_listhead_delete4(node,NULL);

    i = zc_listhead_find((zcListHead*)root, (void*)300, mylistnode_cmp, (zcListHead**)&node);
    assert(node == NULL);
    
    root = (MyListNode*)zc_listhead_reverse((zcListHead*)root);
    
    zc_listhead_foreach(root, node) {
        ZCINFO("node: %p %d, next:%p\n", node, node->v, node->next);
    }

    ZCINFO("root->v: %d\n", root->v);
    assert(root->v == 999);

    return 0;
}

typedef struct
{
    ZC_LIST_HEAD
    char    str[64];
}MyStrNode;

int test_listnode_str()
{
    int  i;
    char buf[64];
    MyStrNode *root = NULL, *node;
   
    for (i = 0; i < 1000; i++) {
        node = (MyStrNode*)zc_listhead_new(MyStrNode); 
        sprintf(node->str, "x%d", i);
        
        if (root) {
            zc_listhead_add_tail((zcListHead*)node, (zcListHead*)root);
        }else{
            root = node;
        }
    }

    node = root;
    i = 0;
    zc_listhead_foreach_self(node) {
        sprintf(buf, "x%d", i);
        assert(strcmp(buf, node->str) == 0);
        i++;
    }
    
    sprintf(buf, "x%d", 10);
    node = (MyStrNode*)zc_listhead_pos((zcListHead*)root , 10);
    assert(node != NULL);
    assert(strcmp(buf, node->str) == 0);
    
    sprintf(buf, "x%d", 10);
    zc_listhead_find((zcListHead*)root, (void*)buf, zc_cmp_str, (zcListHead**)&node);
    assert(node != NULL);
    assert(strcmp(buf, node->str) == 0);

    i = zc_listhead_index((zcListHead*)root, (zcListHead*)node);
    assert(i == 10);

    root = (MyStrNode*)zc_listhead_unlink((zcListHead*)root, (zcListHead*)node);
    assert(root != NULL);
    zc_listhead_delete4(node,NULL);

    i = zc_listhead_find((zcListHead*)root, (void*)buf, zc_cmp_int, (zcListHead**)&node);
    assert(node == NULL);
    
    sprintf(buf, "x%d", 999);
    root = (MyStrNode*)zc_listhead_reverse((zcListHead*)root);
    assert(strcmp(root->str, buf) == 0);


    return 0;
}

int test_list()
{
    zcList     *list;
    zcListNode *node;
    //int ret;
    int oldsize;

    list = zc_list_new();
    assert(list != NULL);

    long i;
    for (i = 0; i < 100; i++) {
        //node = zc_listnode_new_data((void*)i);
        assert(zc_list_append(list, (void*)i) == ZC_OK);
        //zc_check(node);
    }
    
    ZCINFO("list size:%d\n", list->size);
    assert(list->size == 100);

    long data;
    i = 0;
    zcListNode *head = list->head, *hnode;

    /*hnode = head;
    for (i=0; i<100; i++) {
        ZCINFO("foreach %ld, head:%p data:%ld\n", i, list->head, hnode->data);
        hnode = (zcListNode*)hnode->next;
    }*/

    zc_listhead_foreach(head, hnode) {
        ZCINFO("foreach %ld, head:%p data:%ld\n", i, list->head, (long)hnode->data);
    }

    i = 0;
    zc_list_foreach(list, data) {
        ZCINFO("foreach %ld, head:%p data:%ld\n", i, list->head, (long)data);
        assert(data == i);
        i++;
    }

    ZCINFO("count:%ld first:%p\n", i, list->head);

    for (i = 0; i < 100; i++) {
        //node = zc_list_at(list, i);
        //assert(node != NULL);
        //assert((long)(node->data) == i);
        data = (long)zc_list_at(list, i, (void*)-1);
        assert(data != -1);
        assert(data == i);
    }

    for (i = 0; i < 100; i++) {
        data = (long)zc_list_pop_front(list);
        //assert(node != NULL);
        assert(data == i);
        //zc_listnode_delete4(node, list);
    }

    for (i = 0; i < 100; i++) {
        //node = zc_listnode_new_data((void*)i);
        assert(zc_list_prepend(list, (void*)i) == ZC_OK);
    }

    zc_list_foreach(list,data) {
        ZCINFO("data:%ld\n", data);
    }
    
    for (i = 0; i < 100; i++) {
        data = (long)zc_list_pop_back(list);
        //assert(node != NULL);
        assert(data == i);
        //zc_listnode_delete4(node, list);
    }

    for (i = 0; i < 100; i++) {
        //node = zc_listnode_new_data((void*)i);
        assert(zc_list_append(list, (void*)i) == ZC_OK);
    }
 
   
    long v = 1000;
    //node = zc_listnode_new_data((void*)v);
    assert(zc_list_append(list, (void*)v) == 0);
    assert(list->size == 101);

    zc_list_remove(list, (void*)v);
    /*zc_check(list);
    assert(zc_list_unlink(list, node) == ZC_OK);
    zc_check(list);*/

    //zc_listnode_delete4(node, list); 
    //assert(node == NULL);

    assert(zc_list_set_cmp(list, zc_cmp_int) == ZC_OK);
    
    v = 1;
    //zc_list_print(list);
    oldsize = list->size;
    node = zc_list_unlink_data(list, (void*)v);
    //zc_list_print(list);
    assert(node != NULL);
    assert(node->data == (void*)1);
    ZCINFO("size: %d\n", list->size);
    assert(list->size == oldsize-1);
    
    zc_listnode_delete4(node, list);
   
    i = 10; 
    //node = zc_list_get(list, i);
    //assert(node != NULL);
    //assert(node->data == (void*)(i+1));
    oldsize = list->size;
    data = (long)zc_list_get(list, i, (void*)-1);
    assert(data == i+1);
    assert(list->size == oldsize-1);
    
    oldsize = list->size;
    assert(zc_list_insert(list, (void*)data, i) == ZC_OK);
    assert(list->size == oldsize+1); 
    assert(zc_list_index(list, (void*)data) == i);
    
    //zcListNode *node2;
    //node2 = zc_list_at(list, i);
    //assert(node2 == node);

    v = 0;
    long vx[3] = {0, 2, 90};
    int thesize = list->size; 
    for (i=0; i<3; i++) {
        //zc_list_print(list);
        //ZCINFO("find %ld, size:%d\n", vx[i], list->size);
        assert(zc_list_find(list, (void*)vx[i]) != NULL);
        if (i == 0) {
            assert(zc_list_remove_pos(list, 0) == ZC_OK);
        }else if (i == 1) {
            assert(zc_list_remove(list, (void*)vx[i]) == ZC_OK);
        }else{
            zcListNode *node2 = zc_list_find(list, (void*)vx[i]);
            assert(node2 != NULL);
            //ZCINFO("1 pos:%d\n", list->size);
            assert(zc_list_remove_node(list, node2) == ZC_OK);
            //ZCINFO("2 pos:%d\n", list->size);
        }
        //ZCINFO("i:%ld size:%d thesize:%d\n", i, list->size, thesize);
        assert(list->size == thesize-i-1);
        assert(zc_list_find(list, (void*)vx[i]) == NULL);
    }

    oldsize = list->size;
    zc_list_reverse(list);

    assert(oldsize == list->size); 
    data = (long)zc_list_at(list, 0, (void*)-1);
    assert(data == 99);

    zc_list_print(list);
    data = (long)zc_list_at(list, -1, (void*)-1);
    ZCINFO("at -1: %ld\n", data);
    assert(data == 3);

    zc_list_delete(list);

    list = zc_list_new();
    assert(zc_list_set_cmp(list, zc_cmp_str) == ZC_OK);

    char buf[64];
    for (i = 0; i < 100; i++) {
        sprintf(buf, "x%ld", i);
        assert(zc_list_append(list, zc_strdup(buf,0)) == ZC_OK);
    }
    //ZCINFO("buf: %p, %p, %s\n", &buf, buf, buf); 
    //char *b = buf;
    assert(zc_list_find(list, buf) != NULL);
    
    zc_list_delete(list);

    return 0;
}

int test_list2()
{
    zcList     *list;
    //zcListNode *node;

    list = zc_list_new();
    assert(list != NULL);
    int ret;
    long i;
    for (i = 0; i < 100; i++) {
        //node = zc_listnode_new_data((void*)i);
        ret = zc_list_insert(list, (void*)i, -1);
        assert(ret == ZC_OK);
        zc_check(list->head->prev);
    }

    assert(list->size == 100);
 
    for (i = 0; i < 100; i++) {
        //node = zc_listnode_new_data((void*)i);
        ret = zc_list_insert(list, (void*)i, -2);
        assert(ret == ZC_OK);
        //zc_check(list->head->prev);
    }

   
    /*
    i = 1;
    long val;
    zc_list_foreach(list, val) {
        if (i == 100) {
            assert(val == 0);
        }else{
            assert(val == i);
        }
        i++; 
    }*/

    return 0;
}    
 
int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW); 
    zc_log_new("stdout", ZC_LOG_ALL);

    ZCINFO("==== test mynode ====\n");
    test_listnode();
    ZCINFO("==== test strnode ====\n");
    test_listnode_str();
    ZCINFO("==== test list ====\n");
    test_list();
    ZCINFO("==== test list2 ====\n");
    test_list2();

    return 0;
}
