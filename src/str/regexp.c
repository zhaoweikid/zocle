/**
 * @file regexp.c
 * @author zhaowei
 * @brief 正则表达式匹配
 */
#ifdef ZOCLE_WITH_PCRE
#include <string.h>
#include <zocle/log/logfile.h>
#include <zocle/str/regexp.h>
#include <zocle/ds/list.h>
#include <zocle/mem/alloc.h>
#include <zocle/str/string.h>

zcRegMatch*   
zc_regmatch_new()
{
    zcRegMatch *rm = (zcRegMatch*)zc_malloc(sizeof(zcRegMatch));
    memset(rm, 0, sizeof(zcRegMatch));

    return rm;
}

void 
zc_regmatch_delete(void *r)
{
    zcRegMatch *rm = (zcRegMatch*)r;
    if (rm->result) {
        zc_free(rm->result);
    }
    if (rm->groups) {
        zc_list_delete(rm->groups);
    }
    if (rm->vec) {
        zc_free(rm->vec);
    }
    zc_free(rm);
}

int 
zc_regmatch_group_num(zcRegMatch *rm)
{
    if (rm->groups)
        return rm->groups->size;
    return 0;
}

char*
zc_regmatch_group_at(zcRegMatch *rm, int pos)
{
    if (rm->groups)
        return zc_list_at(rm->groups, pos, NULL);
    return NULL;
}

char*
zc_regmatch_result(zcRegMatch *rm)
{
    return rm->result;
}

void
zc_regmatch_range(zcRegMatch *rm, int *start, int *end)
{
    *start = rm->start;
    *end   = rm->end;
}

/**
 * 创建正则表达式对象
 * @param res 正则表达式字符串
 */
zcRegExp*    
zc_regexp_new (char *res, int options)
{
    zcRegExp    *reo;

    reo = (zcRegExp*)zc_malloc(sizeof(zcRegExp));
    memset(reo, 0, sizeof(zcRegExp));

    if (zc_regexp_init(reo, res, options) != ZC_OK) {
        zc_free(reo);
        return NULL;
    }
    return reo;
}

void
zc_regexp_delete(void *x)
{
    zc_regexp_destroy(x);
    zc_free(x);
}

int
zc_regexp_init(zcRegExp *reo, char *res, int options)
{
    const char  *pcre_error;
    int     pcre_erroffset;
    int     info, where;

    reo->re = pcre_compile(res, options, &pcre_error, &pcre_erroffset, NULL);
    if (NULL == reo->re) {
        ZCERROR("pcre error in regex '%s': %s at offset %d.\n", res, pcre_error, pcre_erroffset);
        return ZC_ERR;
    }

    reo->extra = pcre_study(reo->re, 0, (const char **)&pcre_error);
    if (NULL != pcre_error) {
        ZCERROR("pcre_study error: %s\n", pcre_error);
        return ZC_ERR;
    }
    info = pcre_fullinfo(reo->re, reo->extra, PCRE_INFO_CAPTURECOUNT, &where);
    if (0 == info) {
        reo->vecsize = (where+2)*3;
        //ZCINFO("vecsize: %d\n", reo->vecsize);
    }else{
        ZCERROR("pcre_fullinfo error!\n");
        return ZC_ERR;
    }
    
    return ZC_OK;
}

void
zc_regexp_destroy (void *x)
{
    zcRegExp *reo = (zcRegExp*)x;
    pcre_free(reo->extra);
    pcre_free(reo->re);
}


/**
 * 正则表达式匹配
 * @param reo 正则表达式对象
 * @param s 待匹配字符串
 * @param slen 待匹配字符串长度
 * @param offset 匹配中的偏移，给zc_regexp_findall用的
 */
int
zc_regexp_match_ (zcRegExp *reo, const char *s, int slen, int offset, int **vecx)
{
    int     count;
    int     *vec;

    if (reo->vecsize == 0)
        return -2;

    vec = (int*)zc_malloc(sizeof(int) * reo->vecsize);
    memset(vec, 0, sizeof(int) * reo->vecsize);
    count = pcre_exec(reo->re, reo->extra, s, slen, offset, 0, vec, reo->vecsize);
    if (count < 0) {
        zc_free(vec);
        *vecx = NULL;
    }else{
        *vecx = vec;
    }

    return count;
}

static zcRegMatch*
zc_regexp_vec2match(zcRegExp *reo, const char *s, int *vec, int count)
{
    zcRegMatch *rm = zc_regmatch_new();
    rm->start = vec[0];
    rm->end = vec[1];
    rm->vec = vec;

    int firstlen = vec[1] - vec[0];
    rm->result = zc_strdup(&s[vec[0]], firstlen);
    //ZCINFO("first len:%d, %s\n", firstlen, rm->result);

    if (count > 1) {
        rm->groups = zc_list_new();
        rm->groups->del = zc_free_func;

        int i, n;
        int  vn = reo->vecsize / 2;
        for (i=2; i<vn*2; i+=2) {
            if (vec[i+1] == 0)
                break;
            n = vec[i+1]-vec[i];
            if (n == 0) {
                zc_list_append(rm->groups, zc_strdup("", 0));
            }else{
                //char *x = zc_strdup(&s[vec[i]], n);
                zc_list_append(rm->groups, zc_strdup(&s[vec[i]], n));
            }
        }
    }
    return rm;
}

zcRegMatch*
zc_regexp_search (zcRegExp *reo, const char *s, int slen)
{
    int     count;
    int     *vec;
    
    count = zc_regexp_match_(reo, s, slen, 0, &vec);
    if (count >= 0) {
        return zc_regexp_vec2match(reo, s, vec, count);
    }
    return NULL;
}

/** 
 * 在字符串中匹配出所有符合该正则的字符串
 * @param res 正则表达式对象
 * @param s 待匹配字符串
 * @param slen 待匹配字符串长度
 * @return 返回匹配成功的个数
 */
zcList*
zc_regexp_findall (zcRegExp *reo, const char *s, int slen)
{
    int ret;
    int offset = 0;
    int *vec;
    zcList *list = NULL;

    while ((ret = zc_regexp_match_(reo, s, slen, offset, &vec)) > 0) {
        if (NULL == list) {
            list = zc_list_new();
            list->del = zc_regmatch_delete;
        }

        zcRegMatch *rm = zc_regexp_vec2match(reo, s, vec, ret);
        zc_list_append(list, rm);
        offset = rm->end;
    }

    return list;
}



zcList*
zc_regexp_split (zcRegExp *reo, const char *s, int slen)
{
    int ret;
    int offset = 0;
    int start = 0;
    int *vec;
    char *ps;
    zcList *list = NULL;

    list = zc_list_new();
    list->del = zc_free_func;

    while ((ret = zc_regexp_match_(reo, s, slen, offset, &vec)) > 0) {
        if (vec[0]-start == 0) {
            ps = zc_malloc(1);
            ps[0] = 0;
        }else{
            ps = zc_strdup(&s[start], vec[0]-start);
        }
        start = vec[1];
        zc_list_append(list, ps);
        offset = vec[1];
    }
    ps = zc_strdup(&s[start], slen-start);
    zc_list_append(list, ps);

    return list;
} 

#endif

