#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <zocle/zocle.h>

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);

#ifdef ZOCLE_WITH_PCRE
    char *restr1 = "^[0-9]+$";
    char *restr2 = "=([0-9]+)=";

    zcRegExp *re1 = zc_regexp_new(restr1, 0);
    zcRegExp *re2 = zc_regexp_new(restr2, 0);

    //char *test1 = "fafasdfasfjaiod";
    //char *test2 = "fafasdf324asfjaio222sdadf";
    char *test3 = "fafasdf=324=asfjaio=23=2sdadf";
    char *test4 = "2123124";
    char *test5 = "=1=fafasdf=324=asfjaio=23=2sdadf=2=";

    zcRegMatch *rm = zc_regexp_search(re1, test4, strlen(test4));
    if (NULL == rm) {
        ZCINFO("not search.\n");
    }else{
        ZCINFO("found! start:%d end:%d group:%d %s\n", rm->start, rm->end, 
                    zc_regmatch_group_num(rm), rm->result);
        zc_regmatch_delete(rm);
    }

    ZCINFO("=====================\n");
    rm = zc_regexp_search(re2, test3, strlen(test3));
    if (NULL == rm) {
        ZCINFO("not search.\n");
    }else{
        ZCINFO("found! start:%d end:%d group:%d %s\n", rm->start, rm->end, 
                    zc_regmatch_group_num(rm), rm->result);
        if (zc_regmatch_group_num(rm) > 0) {

        }
        zc_regmatch_delete(rm);
    }
    ZCINFO("=====================\n");
    zcSList *sl = zc_regexp_findall(re2, test3, strlen(test3));
    if (NULL == sl) {
        ZCINFO("not search.\n");
    }else{
        zc_slist_foreach(sl, rm) {
            ZCINFO("found! start:%d end:%d group:%d %s\n", rm->start, rm->end, 
                    zc_regmatch_group_num(rm), rm->result);
            if (zc_regmatch_group_num(rm) > 0) {
            }
            //zc_regmatch_delete(rm);
        }
    }
    ZCINFO("=====================\n");
    zcSList *s2 = zc_regexp_split(re2, test5, strlen(test5));
    if (NULL == sl) {
        ZCINFO("not search.\n");
    }else{
        char *rs;
        zc_slist_foreach(s2, rs) {
            ZCINFO("split result:%s\n", rs);
        }
    }
    
    zc_regexp_delete(re1);
    zc_regexp_delete(re2);
#endif    
    return 0;
}
