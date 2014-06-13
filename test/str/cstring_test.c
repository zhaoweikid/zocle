#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <zocle/zocle.h>

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);

    zcCString a[sizeof(zcCString) + 10];

    zc_cstr_init(a, 10);
    zc_cstr_format(a, "haha:%d", 11);
    zc_cstr_print(a); 

    zcCString *ss = zc_cstr_new_char("aaaa", 0);
    zcSList *ls = zc_cstr_split(ss, ":", 1);
    //ZCINFO("ls:%p, %d\n", ls, ss->size);
    assert(ls != NULL);
    assert(ls->size == 1);
    zc_cstr_delete(ss);
    zc_slist_delete(ls);

    ss = zc_cstr_new_char("aaaa:bbbb", 0);
    ls = zc_cstr_split(ss, ":", 1);
    //ZCINFO("ls:%p, %d\n", ls, ss->size);
    assert(ls != NULL);
    assert(ls->size == 2);
    zc_cstr_delete(ss);
    zc_slist_delete(ls);

    ss = zc_cstr_new_char("aaaa:bbbb:ccc", 0);
    ls = zc_cstr_split(ss, ":", 1);
    //ZCINFO("ls:%p, %d\n", ls, ss->size);
    assert(ls != NULL);
    assert(ls->size == 2);
    zc_cstr_delete(ss);
    zc_slist_delete(ls);

    ss = zc_cstr_new_char("aaaa:bbbb:ccc", 0);
    ls = zc_cstr_split(ss, ":", 0);
    //ZCINFO("ls:%p, %d\n", ls, ss->size);
    assert(ls != NULL);
    assert(ls->size == 3);
    zc_cstr_delete(ss);
    zc_slist_delete(ls);

    ss = zc_cstr_new_char(":aaaa:bbbb:ccc:", 0);
    ls = zc_cstr_split(ss, ":", 0);
    //ZCINFO("ls:%p, %d\n", ls, ss->size);
    assert(ls != NULL);
    assert(ls->size == 5);
   
    //assert(zc_slist_first_data(ls)
    zcCString *ss1 = zc_slist_first(ls);
    ZCINFO("first size:%d len:%d %s\n", ss1->size, ss1->len, ss1->data);
    assert(ss1->len == 0);

    zcCString *ss2 = zc_slist_last(ls);
    ZCINFO("last size:%d len:%d %s\n", ss1->size, ss1->len, ss1->data);
    assert(ss2->len == 0);

    zc_cstr_delete(ss);
    zc_slist_delete(ls);



    return 0;
}
