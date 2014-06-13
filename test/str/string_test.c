#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <zocle/zocle.h>

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);
    //ZCWARN("====== string test ======\n");
    zcString *s, *s1;

    s = zc_str_new(0);
    assert(s != NULL);
    assert(s->size == 32);
    assert(s->len == 0);
    zc_str_delete(s);

    s = zc_str_new_char("haha", 4); 
    assert(strcmp(s->data, "haha") == 0); 
    assert(s->size == 5);
    assert(s->len == 4);
    zc_str_delete(s);

    s = zc_str_format(NULL, "gogo: %d\n", 100);
    assert(s != NULL);
    assert(s->size >= 12);
    assert(s->len == 10);
    assert(strcmp(s->data, "gogo: 100\n") == 0);
    assert(zc_str_len(s) == 10);
  
    s1 = NULL;
    s1 = zc_str_copy(s1, s);
    assert(s1 != NULL);
    assert(s1->len == 10);
    assert(strcmp(s1->data, "gogo: 100\n") == 0);

    int oldsize = s->size;
    zc_str_clear(s);
    assert(s->data[0] == 0);
    assert(s->size == oldsize);
    assert(s->len == 0);

    zc_str_assign(s, "test", 4); 
    assert(s->size == oldsize);
    assert(s->len == 4);
    assert(strcmp(s->data, "test") == 0);

    zc_str_append(s, "gogo"); 
    //zc_str_print(s);
    assert(s->size == oldsize);
    assert(s->len == 8);
    assert(strcmp(s->data, "testgogo") == 0);

    zc_str_append_len(s, "haha", 4);
    assert(s->size >= 13);
    assert(s->len == 12);
    assert(strcmp(s->data, "testgogohaha") == 0);

    zc_str_append_c(s, '1');
    assert(s->size >= 14);
    assert(s->len == 13);
    assert(strcmp(s->data, "testgogohaha1") == 0);

    zc_str_insert(s, 0, "22", 2); 
    assert(s->size >= 16);
    assert(s->len == 15);
    assert(strcmp(s->data, "22testgogohaha1") == 0);

    zc_str_insert(s, 2, "33", 2); 
    assert(s->size >= 18);
    assert(s->len == 17);
    assert(strcmp(s->data, "2233testgogohaha1") == 0);

    zc_str_insert(s, 17, "44", 2);
    assert(s->size >= 20);
    assert(s->len == 19);
    assert(strcmp(s->data, "2233testgogohaha144") == 0);

    zc_str_insert(s, 170, "55", 2);
    assert(s->size >= 22);
    assert(s->len == 21);
    assert(strcmp(s->data, "2233testgogohaha14455") == 0);

    oldsize = s1->size;
    zc_str_truncate(s1, 4);
    assert(s1->size == oldsize);
    assert(s1->len == 4);
    assert(strcmp(s1->data, "gogo") == 0);

    zcString *ns, *ns2;
    ns = zc_str_replace(s, "ha", "AhAh", 0);
    //zc_str_print(ns);
    assert(ns->size >= 26);
    assert(ns->len == 25);
    assert(strcmp(ns->data, "2233testgogoAhAhAhAh14455") == 0);

    ns2 = zc_str_replace_case(ns, "ah", "ah", 1);
    assert(ns2->size >= 26);
    assert(ns2->len == 25);
    //ZCINFO("data:%s\n", ns2->data);
    assert(strcmp(ns2->data, "2233testgogoahAhAhAh14455") == 0);
    zc_str_delete(ns2);

    ns2 = zc_str_replace_case(ns, "ah", "ah", 3);
    assert(ns2->size >= 26);
    assert(ns2->len == 25);
    assert(strcmp(ns2->data, "2233testgogoahahahAh14455") == 0);
    zc_str_delete(ns2);

    ns2 = zc_str_replace_case(ns, "ah", "ah", 0);
    //zc_str_print(ns2);
    assert(ns2->size >= 26);
    assert(ns2->len == 25);
    assert(strcmp(ns2->data, "2233testgogoahahahah14455") == 0);
    zc_str_delete(ns2);

    ns2 = zc_str_remove(ns, "ah");
    assert(ns2->size >= 26);
    assert(ns2->len == 25);
    assert(strcmp(ns2->data, "2233testgogoAhAhAhAh14455") == 0);
    zc_str_delete(ns2);

    ns2 = zc_str_remove_case(ns, "ah");
    assert(ns2->size >= 18);
    assert(ns2->len == 17);
    assert(strcmp(ns2->data, "2233testgogo14455") == 0);
    zc_str_delete(ns2);
    //zc_str_delete(ns);
 
    int pos;
    pos = zc_str_find(s, "ha");
    assert(pos == 12);
 
    pos = zc_str_find(ns, "ha");
    assert(pos == ZC_ERR_NOT_FOUND);
    pos = zc_str_find_case(ns, "ha");
    assert(pos == 13);
   
    pos = zc_str_find(s, "2233");
    assert(pos == 0);

    pos = zc_str_find(s, "2222");
    assert(pos == ZC_ERR_NOT_FOUND);

    int oldlen = s->len;
    oldsize = s->size;

    zc_str_cap(s); 
    assert(s->size == oldsize);
    assert(s->len == oldlen);
    assert(strcmp(s->data, "2233TESTGOGOHAHA14455") == 0);

    zc_str_low(s); 
    assert(s->size == oldsize);
    assert(s->len == oldlen);
    assert(strcmp(s->data, "2233testgogohaha14455") == 0);

    zc_str_reverse(s);
    assert(s->size == oldsize);
    assert(s->len == oldlen);
    assert(strcmp(s->data, "55441ahahogogtset3322") == 0);

    ns2 = zc_str_dup(s);
    //zc_str_print(ns2);
    assert(ns2->size == oldsize);
    assert(ns2->len == oldlen);
    assert(strcmp(ns2->data, "55441ahahogogtset3322") == 0);
    
    zc_str_append(ns2, "55");
    //zc_str_print(ns2);
    assert(ns2->size >= oldlen + 2);
    assert(ns2->len == oldlen + 2);
    assert(strcmp(ns2->data, "55441ahahogogtset332255") == 0);
    
    zc_str_trim(ns2, '5', 0);
    //zc_str_print(ns2);
    assert(ns2->size == oldsize);
    assert(ns2->len == oldlen - 2);
    assert(strcmp(ns2->data, "441ahahogogtset3322") == 0);

    zc_str_strip(ns2, "1234", 0);
    //zc_str_print(ns2);
    assert(ns2->size == oldsize);
    assert(ns2->len == oldlen - 9);
    assert(strcmp(ns2->data, "ahahogogtset") == 0);

    int ret;
    ret = zc_str_cmp(s, ns2, 0);
    assert(ret != 0);
    zc_str_delete(ns2);
    

    ns2 = zc_str_dup(s);
    zc_str_cap(ns2);
    ret = zc_str_cmp_case(s, ns2, 0);
    assert(ret == 0);
    ret = zc_str_cmp(s, ns2, 0);
    assert(ret != 0);
    zc_str_truncate(ns2, ns2->len - 2);
    ret = zc_str_cmp_case(s, ns2, 0);
    assert(ret != 0);
    ret = zc_str_cmp_case(s, ns2, ns2->len - 2);
    assert(ret == 0);
    zc_str_delete(ns2);
    zc_str_delete(ns);
    
    ns2 = zc_str_sub(s, 0, 4);
    assert(ns2->size > 4);
    assert(ns2->len == 4);
    assert(strcmp(ns2->data, "5544") == 0);
    zc_str_delete(ns2);

    ns2 = zc_str_sub(s, -4, 4);
    assert(ns2->size > 4);
    assert(ns2->len == 4);
    assert(strcmp(ns2->data, "3322") == 0);
    zc_str_delete(ns2);

    ret = zc_str_sub_count(s, 0, 0, "ah");
    assert(ret == 2);
    
    zc_str_reverse(s);
    zc_str_cap(s);
    //zc_str_print(s); 

    ret = zc_str_sub_count_case(s, 0, 0, "go");
    assert(ret == 2);
    
    ret = zc_str_sub_count_case(s, 10, 0, "go");
    assert(ret == 1);
   
    zcString    *a1, *a2, *a3;
    a1 = zc_str_new_char("haha", 4);
    a2 = zc_str_new_char("gogo", 4);
    a3 = zc_str_new_char("1234", 4);

    ns = zc_str_join("|", a1, a2, a3, NULL); 
    assert(ns->size > 14);
    assert(ns->len == 14);
    assert(strcmp(ns->data, "haha|gogo|1234") == 0);
    zc_str_delete(a1);
    zc_str_delete(a2);
    zc_str_delete(a3);
    zc_str_delete(ns);

    ns = zc_str_join_char("|", "222", "python", "123", NULL);
    assert(ns->size > 14);
    assert(ns->len == 14);
    assert(strcmp(ns->data, "222|python|123") == 0);
    zc_str_delete(ns);
  

    /*
    ns = zc_str_new_char("\"haha\"", 6);
    zc_str_print(ns);
    ns2 = zc_str_quote(ns);
    zc_str_print(ns2);
    assert(ns2->len == 8);
    assert(strcmp(ns2, "\\\"haha\\\"") == 0);
    */

    zc_str_delete(s);


    zcString *ss = zc_str_new_char("aaaa", 0);
    zcSList *ls = zc_str_split(ss, ":", 1);
    //ZCINFO("ls:%p, %d\n", ls, ss->size);
    assert(ls != NULL);
    assert(ls->size == 1);
    zc_str_delete(ss);
    zc_slist_delete(ls);

    ss = zc_str_new_char("aaaa:bbbb", 0);
    ls = zc_str_split(ss, ":", 1);
    //ZCINFO("ls:%p, %d\n", ls, ss->size);
    assert(ls != NULL);
    assert(ls->size == 2);
    zc_str_delete(ss);
    zc_slist_delete(ls);

    ss = zc_str_new_char("aaaa:bbbb:ccc", 0);
    ls = zc_str_split(ss, ":", 1);
    //ZCINFO("ls:%p, %d\n", ls, ss->size);
    assert(ls != NULL);
    assert(ls->size == 2);
    zc_str_delete(ss);
    zc_slist_delete(ls);

    ss = zc_str_new_char("aaaa:bbbb:ccc", 0);
    ls = zc_str_split(ss, ":", 0);
    //ZCINFO("ls:%p, %d\n", ls, ss->size);
    assert(ls != NULL);
    assert(ls->size == 3);
    zc_str_delete(ss);
    zc_slist_delete(ls);

    ss = zc_str_new_char(":aaaa:bbbb:ccc:", 0);
    ls = zc_str_split(ss, ":", 0);
    //ZCINFO("ls:%p, %d\n", ls, ss->size);
    assert(ls != NULL);
    assert(ls->size == 5);
   
    zcString *ss1 = zc_slist_first(ls);
    ZCINFO("first size:%d len:%d %s\n", ss1->size, ss1->len, ss1->data);
    assert(ss1->len == 0);

    zcString *ss2 = zc_slist_last(ls);
    ZCINFO("last size:%d len:%d %s\n", ss1->size, ss1->len, ss1->data);
    assert(ss2->len == 0);

    zc_str_delete(ss);
    zc_slist_delete(ls);




    return 0;
}
