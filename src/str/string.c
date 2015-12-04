/**
 * @file zstring.c
 * @author zhaowei
 * @brief 字符串
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <zocle/log/logfile.h>
#include <zocle/base/defines.h>
#include <zocle/str/string.h>

#define  ZC_STRING_CHECK(sstr) 

/*#define  ZC_STRING_CHECK(sstr) \
    do{\
    if (NULL == sstr) {\
        return ZC_ERR_NULL;\
    }\
    if (0 == sstr->len) {\
        return ZC_ERR_EMPTY;\
    }\
    }while(0)
*/

char*
zc_strdup(const char* s, int len)
{
    char    *ns;
    
    if (len <= 0)
        len = strlen(s);
    ns = (char *)zc_malloc(len + 1);
    memcpy(ns, s, len);
    ns[len] = 0;

    return ns;
}

char*   
zc_strnstr(const char *s1, const char *s2, int len)
{
    size_t l1, l2;

    l2 = strlen(s2);
    if (!l2)
        return (char *)s1;
    l1 = strlen(s1);
    if (l1 > len)
        l1 = len;
    while (l1 >= l2) {
        l1--;
        if (!memcmp(s1, s2, l2))
            return (char *)s1;
        s1++;
    }

    return NULL;
}

char*   
zc_strnchr(const char *s, const char c, int len)
{
    int i;
    for (i=0; i<len; i++) {
        if (s[i] == c)
            return (char*)(s+i);
    }
    return NULL;
}



#if defined(__sun)  || defined (__CYGWIN__) || defined (_WIN32)

char *strcasestr(const char *s, const char *find)
{
    register char c, sc;
    register size_t len;

    if ((c = *find++) != 0) {
        len = strlen(find);
        do {
            do {
                if ((sc = *s++) == 0)
                    return (NULL);
            } while (sc != c);
        } while (strncasecmp(s, find, len) != 0);
        s--;
    }
    return ((char *) s);
}

#endif



/**
 * 计算字符串结构重新分配内部的存储空间应该分配的实际大小
 * @param sstr    字符串结构指针
 * @param newsize 新的内部存储空间大小
 * @return        实际应该分配的空间大小
 */
int
zc_str_resize_size(zcString*  sstr, int newsize)
{
    int  step = 256;

    if (sstr->size > step) {
        if (sstr->size > 102400) {
            step = sstr->size * 0.5;
        }else{
            step = sstr->size;
        }
    }

    int  r1 = newsize / step;
    int  n1 = r1 * step;
    if (n1 < newsize)
        return (r1+1)*step;
    else
        return r1*step;
}

/**
 * 为字符串结构重新分配存储空间,不保留原来的数据
 * @param sstr    字符串结构指针
 * @param nsize   新的字符串结构中存储空间大小
 * @return        成功返回ZC_OK
 */
int
zc_str_newsize(zcString* sstr, int nsize)
{
    if (sstr->_onstack) 
        return ZC_ERR;
    int  newsize = zc_str_resize_size(sstr, nsize);

    char *ndata = (char*)zc_malloc(sizeof(char) * newsize);
    zc_free(sstr->data);
    sstr->data = ndata;
    sstr->len = 0;
    sstr->size = newsize;

    return ZC_OK;
}

/**
 * 为字符串结构分配新的内部存储空间，原来的数据会被复制到新的中
 * @param sstr    字符串结构指针
 * @param nsize   字符串新的内部存储空间大小
 * @return        成功返回ZC_OK
 */
int
zc_str_resize(zcString* sstr, int nsize)
{
    int     newsize = zc_str_resize_size(sstr, nsize);
    char    *ndata  = (char*)zc_malloc(sizeof(char) * newsize);
    
    //memset(ndata, 0, newsize);
    memcpy(ndata, sstr->data, sstr->len+1);
    zc_free(sstr->data);
    sstr->data = ndata;
    sstr->size = newsize;

    return ZC_OK;
}

/**
 * 创建新的字符串结构
 * @param 初始化长度
 * @return     成功返回新的字符串结构
 */
zcString*
zc_str_new (int len)
{
    zcString *s;
    if (zc_str_new2(&s, len) != ZC_OK) {
        return NULL;
    }
    return s;
}

void
zc_str_delete(void *x)
{
    if (x) {
        zc_str_destroy(x);
        zcString *s = (zcString*)x;
        zc_free(s);
    }
}

void
zc_str_delete3(void *x, const char *file, int lineno)
{
    if (x) {
        zc_str_destroy(x);
        zcString *s = (zcString*)x;
        zc_free3(s, file, lineno);
    }
}

/**
 * 创建新的字符串结构
 * @param step 字符串结构内部存储每次增长的大小
 * @param err  返回的出错信息
 * @return     成功返回新的字符串结构
 */
int
zc_str_new2 (zcString **s, int len)
{
    zcString  *sstr;
    
    sstr = (zcString *)zc_malloc(sizeof(zcString));
    zc_str_init(sstr, len);

    *s = sstr;
    return ZC_OK;
}

/**
 * 创建新的字符串结构
 * @param s 待初始化的字符串
 * @param len 初始化长度
 * @return     成功返回新的字符串结构
 */
zcString*   
zc_str_new_char(char *str, int len)
{
    zcString *s;
    
    if (NULL == str)
        return NULL;

    if (len <= 0) {
        len = strlen(str);
    }
    s = zc_str_new(len + 1);
    memcpy(s->data, str, len);
    s->data[len] = 0;
    s->len = len;
    s->__type = ZC_STRING;
    
    return s;
}

int
zc_str_init(zcString *s, int len)
{
    memset(s, 0, sizeof(zcString));
    len = (len > 0 ? len: 32);
    s->data = (char *)zc_malloc(len * sizeof(char));
    s->data[0] = 0;
    s->size = len;
    s->__type = ZC_STRING;
    //s->len  = 0;

    return ZC_OK;
}

void
zc_str_init_stack(zcString *s, char *buf, int len)
{
    memset(s, 0, sizeof(zcString));
    s->_onstack = 1;
    s->data = buf;
    s->size = len;
    s->__type = ZC_STRING;
    s->data[0] = 0;
    s->len = 0;
}

void
zc_str_init_stack_exist(zcString *s, char *buf, int len)
{
    memset(s, 0, sizeof(zcString));
    s->_onstack = 1;
    s->data = buf;
    s->size = len;
    s->len  = len;
    s->__type = ZC_STRING;
}

/**
 * @brief 释放字符串结构中占用的资源
 * @param sstr 字符串结构指针
 */
void
zc_str_destroy(void *s)
{
    if (NULL == s) {
        ZCERROR("zc_str_destroy NULL pointer!\n");
    } else {
        zcString  *str = (zcString*)s;
        if (str->_onstack == 0) {
            zc_free(str->data);
        }
    }
    //zc_free(str);
}

void
zc_str_destroy3(void *s, const char *file, int lineno)
{
    if (NULL == s) {
        ZCERROR("zc_str_destroy NULL pointer!\n");
    }
    zcString  *str = (zcString*)s;
    if (str->_onstack == 0) {
        zc_free3(str->data, file, lineno);
    }
    //zc_free(str);
}

/**
 * @brief 输出字符串的内容和属性
 * @param sstr    字符串结构指针
 */
void
zc_str_print(zcString *sstr)
{
    if (NULL == sstr) {
        ZCINFO("string is NULL. %p\n", sstr);
    }else{
        ZCINFO("string size:%d len:%d, data:%p, %s\n", sstr->size, sstr->len, sstr->data, sstr->data);
    }
}

void
zc_str_printr(zcString *sstr)
{
    if (NULL == sstr) {
        ZCINFO("string is NULL. %p\n", sstr);
    }else{
        ZCINFO("string size:%d len:%d, data:%p, %s\n", sstr->size, sstr->len, sstr->data, sstr->data);
    }
}


/**
 * 清除字符串结构内部数据
 * @param sstr 字符串结构指针
 * @return    正常返回ZC_OK
 */
int
zc_str_clear (zcString *sstr)
{
    sstr->len = 0;
    sstr->data[0] = 0;
    
    return ZC_OK;
}

/**
 * 把字符串结构中的内存置0
 * @param sstr    字符串结构指针
 * @return    成功返回 ZC_OK
 */
int
zc_str_zero (zcString *sstr)
{
    memset(sstr->data, 0, sstr->size);
    sstr->len = 0;
    return ZC_OK;
}
 
/**
 * 获取字符串结构中字符串数据的长度
 * @param sstr 字符传结构指针
 * @return 成功返回字符串长度
 */
int
zc_str_len (zcString *sstr)
{
    return sstr->len;
}

/**
 * 把C字符串赋值给字符串结构
 * @param sstr    字符串结构指针
 * @param ccstr   C字符串指针
 * @param cslen   C字符串长度
 * @return    成功返回ZC_OK
 */
int
zc_str_assign (zcString *sstr, const char *ccstr, int cslen)
{
    if (ccstr == NULL) {
        return ZC_ERR_NULL;
    }

    if (cslen <= 0)
        cslen = strlen(ccstr);
    
    if (cslen + 1 > sstr->size) {
        int     ret; 
        ret = zc_str_newsize(sstr, cslen+1);
        if (ret < 0) {
            return ret;
        }
    }
    strncpy(sstr->data, ccstr, cslen);
    sstr->data[cslen] = 0;
    sstr->len = cslen;
    return ZC_OK;
}

zcString*
zc_str_copy(zcString *sstr, zcString *fstr)
{
    if (fstr == NULL || fstr->len == 0)
        return NULL;

    if (sstr == NULL) {
        sstr = zc_str_new(fstr->size);
    }
    zc_str_assign(sstr, fstr->data, fstr->len);
    return sstr;
}

/**
 * 把格式化字符串存储
 */

zcString *
zc_str_format(zcString *s, char *format, ...)
{
    va_list arg;
    char    buf[8192] = {0};
    int     maxlen = sizeof(buf)-1;
    int     done;
    
    if (NULL == s) {
        s = zc_str_new(0);
    }
    
    va_start(arg, format);
    //ZCINFO("va maxlen:%d\n", maxlen);
    done = vsnprintf(buf, maxlen, format, arg);
    buf[done] = 0;
    zc_str_append_len(s, buf, done);
    va_end(arg);

    return s;
}


/**
 * 向字符串结构中追加数据
 * @param sstr    字符串结构指针
 * @param ccstr   C字符串指针
 * @return    成功返回ZC_OK
 */
int
zc_str_append (zcString *sstr, const char *ccstr)
{
    int     isize, cslen; 
    char    *copyto;

    if (ccstr == NULL) {
        return ZC_ERR_NULL;
    }
    
    cslen = strlen(ccstr);
    isize = sstr->len + cslen + 1;
    //ZCINFO("1 size:%d, len:%d, isize:%d copylen:%d\n", sstr->size, sstr->len, isize, cslen);
    if (isize > sstr->size) {
        int     ret; 
        ret = zc_str_resize(sstr, isize);
        if (ret < 0) {
            return ret;
        }
        //ZCINFO("resize:%s\n", sstr->data);
    }
    //ZCINFO("2 size:%d, len:%d, isize:%d copylen:%d\n", sstr->size, sstr->len, isize, cslen);
    copyto = sstr->data + sstr->len;
    memcpy(copyto, ccstr, cslen);
    sstr->len = isize - 1;
    sstr->data[sstr->len] = 0;

    return ZC_OK;
}


/**
 * 向字符串结构中追加格式数据
 * @param sstr    字符串结构指针
 * @param format  格式化字符串
 * @return    成功返回ZC_OK
 */
int
zc_str_append_format(zcString *sstr, char *format, ...)
{
    va_list arg;
    char    buf[8192];
    int     maxlen = 8191;
    int     done;
    int     ret;
    zcString *s;
    
    if (NULL == sstr) {
        return ZC_ERR_NULL;
    }
    s = zc_str_new(1024);
    
    va_start(arg, format);
    done = vsnprintf(buf, maxlen, format, arg);
    buf[done] = 0;
    zc_str_append_len(s, buf, done);
    va_end(arg);

    ret = zc_str_append_len(sstr, s->data, s->len);
    zc_str_delete(s);

    return ret;
}

/**
 * 向字符串结构中追加数据
 * @param sstr    字符串结构指针
 * @param ccstr   C字符串指针
 * @param cslen   C字符串长度
 * @return    成功返回ZC_OK
 */
int
zc_str_append_len (zcString *sstr, const char *ccstr, int cslen)
{
    if (cslen <= 0)
        cslen = strlen(ccstr);

    int isize = sstr->len + cslen + 1;
    if (isize > sstr->size) {
        int ret = zc_str_resize(sstr, isize);
        //ZCINFO("str resize:%d", sstr->size);
        if (ret < 0) {
            return ret;
        }
    }
    
    memcpy(sstr->data+sstr->len, ccstr, cslen);
    sstr->len += cslen;
    sstr->data[sstr->len] = 0;

    return ZC_OK;
}


zcString*
zc_str_append_len_new(zcString *sstr, const char *ccstr, int cslen) 
{
    if (NULL == sstr) {
        return zc_str_new_char((char*)ccstr, cslen);
    }
    zc_str_append_len(sstr, ccstr, cslen);
    return sstr;
}


int 
zc_str_append_until(zcString *sstr, const char *cstr, const char *utilstr)
{
    if (cstr == NULL || utilstr == NULL) {
        return ZC_ERR_NULL;
    }
    const char *end = strstr(cstr, utilstr);
    if (NULL == end) {
        return ZC_ERR;
    }
    return zc_str_append_len(sstr, cstr, end-cstr); 
}


/**
 * 向字符串结构中追加数据
 * @param sstr    字符串结构指针
 * @param ccstr   C字符串指针
 * @return    成功返回ZC_OK
 */
int
zc_str_append_c (zcString *sstr, const char c)
{
    int     len;

    len = sstr->len;
    if (len + 2 > sstr->size) {
        int ret;
        ret = zc_str_resize(sstr, len+2);
        if (ret < 0) {
            return ret;
        }
    }
    sstr->data[len] = c;
    sstr->data[len+1] = 0;
    sstr->len++;
    
    return ZC_OK;
}

int
zc_str_skip_len (zcString *sstr, int cslen)
{
    int     isize; 
    isize = sstr->len + cslen + 1;
  
    if (isize > sstr->size) {
        int     ret; 
        ret = zc_str_resize(sstr, isize);
        if (ret < 0) {
            return ret;
        }
    }
    
    char *copyto = sstr->data + sstr->len;
    memset(copyto, 0, cslen);
    sstr->len = isize - 1;
    sstr->data[sstr->len] = 0;

    return ZC_OK;
}

/**
 * 向字符串结构中插入数据
 * @param sstr    字符串结构指针
 * @param begin   字符串结构中开始插入的位置
 * @param ccstr   待插入的C字符串
 * @param cslen   C字符串长度
 * @return    成功返回ZC_OK
 */
int
zc_str_insert (zcString *sstr, int pos, const char *ccstr, int cslen)
{
    int     isize; 
    char    *cat;
    char    *ndata;

    if (ccstr == NULL) {
        return ZC_ERR_NULL;
    }
    
    if (cslen <= 0)
        cslen = strlen(ccstr);

    if (pos < 0) 
        pos = sstr->len + pos;
    if (pos > sstr->len)
        pos = sstr->len;

    isize = sstr->len + cslen + 1;
    //zc_str_print(sstr);
    if (isize > sstr->size) {
        int     newsize;
        newsize = zc_str_resize_size(sstr, isize);
        ndata = (char *)zc_malloc(newsize);

        strncpy(ndata, sstr->data, pos);
        cat = ndata + pos;
        strncpy(cat, ccstr, cslen);
        cat += cslen;
        //zc_str_print(sstr);
        strncpy(cat, sstr->data + pos, sstr->len - pos);
        sstr->size = newsize; 

        zc_free(sstr->data);
        sstr->data = ndata;
    }else{
        char    *sfrom, *sto;
        int     clen;

        sfrom = sstr->data + pos;
        sto = sfrom + cslen;
        clen = sstr->len - pos;
        memmove(sto, sfrom, clen);
        memcpy(sfrom, ccstr, cslen * sizeof(char));
        sstr->data[isize] = 0;
    } 
   
    sstr->len = isize - 1;
    sstr->data[sstr->len] = 0;
    return ZC_OK;
}

/**
 * 把字符串结构截断为指定长度
 * @param sstr    字符串结构指针
 * @param pos     截断的长度
 * @return        成功返回ZC_OK
 */
int
zc_str_truncate (zcString *sstr, int pos)
{
    if ( (pos < 0) || (pos >= sstr->size)) {
        return ZC_ERR_RANGE;
    }

    sstr->data[pos] = 0;
    sstr->len = pos;

    return ZC_OK;
}


/**
 * 把字符串结构中的指定字符串替换为其他字符串
 * @param sstr    字符串结构指针
 * @param lookup  待替换的字符串内容
 * @param newstr  替换为的字符串
 * @param count   替换的次数
 * @param icase   是否忽略大小写，1不区分，0区分
 * @return        成功返回ZC_OK
 */
zcString*
zc_str_replace_str(zcString *sstr, const char* lookup, const char* newstr, 
                    int count, int icase)
{
    int     currep = 0;
    int     looklen, newstrlen = 0;
    int     found = 0;
    char    *cdata, *rets, *end;
    zcString  *newsstr = NULL;

    if (sstr->len == 0 || NULL == lookup)
        return NULL;
    looklen = strlen(lookup);

    if (newstr)
        newstrlen = strlen(newstr);
  
    cdata = sstr->data;
    end   = sstr->data + sstr->len;

    while (1) {
        if (ZC_STR_NOCASE == icase){
            rets = strcasestr(cdata, lookup);
        }else{
            rets = strstr(cdata, lookup);
        }
        if (NULL == rets){
            if (1 == found){ // copy string end block
                zc_str_append_len(newsstr, cdata, (sstr->data+sstr->len)-cdata);
            }else{
                newsstr = zc_str_copy(newsstr, sstr);
            }
            break;
        }else{
            if (0 == found) { //first, create new dest string
                newsstr = zc_str_new_char(sstr->data, rets - sstr->data);
                found = 1;
            }
            if (newstrlen > 0)
                zc_str_append_len(newsstr, newstr, newstrlen);
            cdata = rets + looklen;
            currep++;

            if (count > 0 && currep == count) {
                zc_str_append_len(newsstr, cdata, end-cdata);
                break;
            }
        }
    }
    return newsstr;
}

zcString*
zc_str_replace(zcString *sstr, const char* lookup, const char* newstr, int count)
{
    return zc_str_replace_str(sstr, lookup, newstr, count, ZC_STR_CASE);
}

zcString*
zc_str_replace_case(zcString *sstr, const char* lookup, const char* newstr, int count)
{
    return zc_str_replace_str(sstr, lookup, newstr, count, ZC_STR_NOCASE);
}

/**
 * 在字符串结构中查找指定字符串
 * @param sstr    字符串结构指针
 * @param from    字符串结构中开始查找的位置
 * @param len     字符串结构中查找的长度
 * @param cstr    查找的字符串
 * @param icase   是否忽略大小写，TRUE为忽略
 * @return        成功返回查找出来在字符串中的位置
 */
int           
zc_str_find_str (zcString *sstr, int from, int len, const char *cstr, int icase)
{
    int     looklen;
    char    *cdata, *ret, *end;

    ZC_STRING_CHECK(sstr);

    looklen = strlen(cstr);
   
    if (from < 0)
        from = sstr->len + from;
    if (from + len >= sstr->len) 
        return ZC_ERR_PARAM;

    cdata = sstr->data + from;
    if (len <= 0) {
        end = sstr->data + sstr->len;
    }else{
        end = sstr->data + len;
    }

    if (ZC_STR_NOCASE == icase) {
        ret = strcasestr(cdata, cstr);
    }else{
        ret = strstr(cdata, cstr);
    }
    
    if (NULL == ret) {
        return ZC_ERR_NOT_FOUND;
    }
    if (ret+looklen > end)
        return ZC_ERR_NOT_FOUND;
    return ret-sstr->data;
}

int
zc_str_find (zcString *sstr, const char *cstr)
{
    return zc_str_find_str(sstr, 0, 0, cstr, ZC_STR_CASE);
}

int
zc_str_find_case (zcString *sstr, const char *cstr)
{
    return zc_str_find_str(sstr, 0, 0, cstr, ZC_STR_NOCASE);
}


/**
 * 从字符串结构中删除指定字符串
 * @param sstr    字符串结构指针
 * @param cstr    查找的字符串
 * @param count   匹配次数
 * @param icase   是否忽略大小写，1为不区分，0为区分
 * @return        成功返回ZC_OK
 */
zcString*
zc_str_remove_str (zcString *sstr, const char *cstr, int count, int icase)
{
    return zc_str_replace_str(sstr, cstr, NULL, count, icase);    
}

zcString*
zc_str_remove (zcString *sstr, const char *cstr)
{
    return zc_str_replace_str(sstr, cstr, NULL, 0, ZC_STR_CASE);    
}

zcString*
zc_str_remove_case (zcString *sstr, const char *cstr)
{
    return zc_str_replace_str(sstr, cstr, NULL, 0, ZC_STR_NOCASE);    
}

/**
 * 把字符串结构中的字符串改为大写
 * @param sstr    字符串结构指针
 * @return 成功返回ZC_OK
 */
int           
zc_str_cap (zcString *sstr)
{
    char    *cdata;
    int     i;

    ZC_STRING_CHECK(sstr);
    cdata = sstr->data;
    for (i = 0; i<sstr->len; i++) {
        if ((cdata[i] >= 'a') && (cdata[i] <= 'z'))
            cdata[i] -= 32;
    }
    return ZC_OK;
}

/**
 * 把字符串结构中的字符串改为小写
 * @param sstr    字符串结构指针
 * @return 成功返回ZC_OK
 */
int           
zc_str_low (zcString *sstr)
{
    char    *cdata;
    int     i;

    ZC_STRING_CHECK(sstr);
    cdata = sstr->data;
    for (i = 0; i<sstr->len; i++) {
        if ((cdata[i] >= 'A') && (cdata[i] <= 'Z'))
            cdata[i] += 32;
    }
    return ZC_OK;
}
/**
 * 把字符串中的字符串反转
 * @param sstr    字符串指针
 * @return    成功返回ZC_OK
 */
int           
zc_str_reverse (zcString *sstr)
{
    char   ch;
    char   *now;
    char   *src;
    
    ZC_STRING_CHECK(sstr);
    src = sstr->data;
    now = sstr->data + sstr->len;

    now--;
    while (now > src) {
        ch    = *src;
        *src = *now;
        *now = ch;

        src++;
        now--;
    }
    return ZC_OK;
}

/**
 * 复制一个字符串结构
 * @param sstr    带复制的字符串结构
 * @param err     出错信息
 * @return    成功返回新的字符串结构
 */
zcString*
zc_str_dup(zcString*  sstr)
{
    zcString  *newstr;

    newstr = zc_str_new(sstr->size);
    if (NULL == newstr)
        return NULL;

    newstr->len  = sstr->len;
    newstr->size = sstr->size;
    memcpy(newstr->data, sstr->data, sstr->size);

    return newstr;
}

/**
 * 去除字符串结构中首尾指定字符
 * @param sstr    字符串结构指针
 * @param ch      要去掉的字符
 * @param direction 需要去掉的位置，小于0去掉头部的，大于0去掉尾部的，等于0头尾都去
 * @return    成功返回ZC_OK
 */
int           
zc_str_trim_(zcString* sstr, char ch, int direction)
{
    char    *cdata;
    int     i;

    ZC_STRING_CHECK(sstr);
    
    cdata = sstr->data;
    if (direction <= ZC_STR_BOTH) {
        i = 0;
        while (cdata[i] == ch)
            i++;
        if (i > 0) {
            memmove(sstr->data, sstr->data+i, sstr->len-i+1);
            sstr->len -= i;
        }
    }
    
    if (direction >= ZC_STR_BOTH) {
        i = sstr->len - 1;
        while (cdata[i] == ch) {
            i--;
        }
        if (i < sstr->len) {
            i++;
            cdata[i] = 0;
            sstr->len = i;
        }
    }
    return ZC_OK;
}

int 
zc_str_trim(zcString* sstr, char ch)
{
    return zc_str_trim_(sstr, ch, ZC_STR_BOTH);
}

int 
zc_str_ltrim(zcString* sstr, char ch)
{
    return zc_str_trim_(sstr, ch, ZC_STR_LEFT);
}

int 
zc_str_rtrim(zcString* sstr, char ch)
{
    return zc_str_trim_(sstr, ch, ZC_STR_RIGHT);
}

/**
 * 去掉字符串结构中的指定字符
 * @param sstr    字符串结构指针
 * @param chs     带去掉的所有字符组成的字符串
 * @param direction 需要去掉的位置，小于0去掉头部的，大于0去掉尾部的，等于0头尾都去
 * @return    成功返回ZC_OK
 */
int
zc_str_strip_(zcString *sstr, char* chs, int direction)
{
    char    *chs_default = " \t\r\n";
    char    *cdata;
    int     i, k;
    int     clen;

    if (NULL == chs)
        chs = chs_default;
    
    clen = strlen(chs);
    cdata = sstr->data;

    if (direction <= ZC_STR_BOTH) {
        i = 0;
        while (1) {
            for (k = 0; k < clen; k++) {
                if (cdata[i] == chs[k])
                    break;
            }
            if (k == clen)
                break;
            i++;
        }
        if (i > 0) {
            memmove(sstr->data, sstr->data+i, sstr->len-i+1);
            sstr->len -= i;
        }
    }
    
    if (direction >= ZC_STR_BOTH) {
        i = sstr->len - 1;
        while (1) {
            for (k = 0; k < clen; k++) {
                if (cdata[i] == chs[k])
                    break;
            }
            if (k == clen)
                break;
            i--;
        }
        if (i < sstr->len) {
            i++;
            cdata[i] = 0;
            sstr->len = i;
        }
    }
    return ZC_OK;

}

int 
zc_str_strip(zcString *sstr, char* chs)
{
    return zc_str_strip_(sstr, chs, ZC_STR_BOTH);
}

int 
zc_str_lstrip(zcString *sstr, char* chs)
{
    return zc_str_strip_(sstr, chs, ZC_STR_LEFT);
}

int 
zc_str_rstrip(zcString *sstr, char* chs)
{
    return zc_str_strip_(sstr, chs, ZC_STR_RIGHT);
}

/**
 * 字符串结构的比较
 * @param lsstr   待比较的字符串结构
 * @param rsstr   待比较的字符串结构
 * @param len     比较的长度
 * @return        相等返回0，行为和strcmp一样
 */
int           
zc_str_cmp (zcString *lsstr, zcString *rsstr, int len)
{
    if (len > 0)
        return strncmp(lsstr->data, rsstr->data, len);
    else
        return strcmp(lsstr->data, rsstr->data);
}

int           
zc_str_cmp_case (zcString *lsstr, zcString *rsstr, int len)
{
    if (len > 0)
        return strncasecmp(lsstr->data, rsstr->data, len);
    else
        return strcasecmp(lsstr->data, rsstr->data);
}



/**
 * 求子字符串
 * @param sstr    字符串结构
 * @param from   子字符串的开始位置
 * @param len   子字符串的长度
 * @return    成功返回新字符串结构
 */
zcString*
zc_str_sub (zcString *sstr, int from, int len)
{
    zcString    *newstr;
    char        *cdata;

    if (0 == sstr->len || len == 0){
        return NULL;
    }
    if (from < 0) {
        from = sstr->len + from;
    }
    if (from >= sstr->len)
        return NULL;

    newstr = zc_str_new(len + 1);
    cdata = sstr->data + from;
    
    zc_str_assign(newstr, cdata, len);
    return newstr;
}

/**
 * 求字符串结构中子字符串的个数
 * @param sstr    字符串结构
 * @param from    开始查找的位置
 * @param len     查找的长度
 * @param lookup  查找的内容
 * @param icase   是否忽略大小写，TRUE/YES为忽略
 * @return        返回出现的次数
 */
int
zc_str_sub_count_str(zcString *sstr, int from, int len, const char* lookup, int icase)
{
    int     looklen;
    char    *cdata, *ret, *end;
    int     icount = 0;

    ZC_STRING_CHECK(sstr);
    if (len < 0)
        return ZC_ERR_PARAM;

    looklen = strlen(lookup);
   
    if (from < 0)
        from = sstr->len + from;
    if (from + len >= sstr->len) 
        return ZC_ERR_PARAM;

    cdata = sstr->data + from;
    if (len == 0) {
        end = sstr->data + sstr->len;
    }else{
        end = sstr->data + len;
    }

    while (1) {
        if (ZC_STR_NOCASE == icase) {
            ret = strcasestr(cdata, lookup);
        }else{
            ret = strstr(cdata, lookup);
        }
        if (NULL != ret) {
            if (ret + looklen > end) {
                break;
            }
            icount++;
            cdata = ret + looklen;
        }else{
            break;
        }
    }

    return icount;
}

int
zc_str_sub_count(zcString *sstr, int from, int len, const char* lookup)
{
    return zc_str_sub_count_str(sstr, from, len, lookup, ZC_STR_CASE);
}

int
zc_str_sub_count_case(zcString *sstr, int from, int len, const char* lookup)
{
    return zc_str_sub_count_str(sstr, from, len, lookup, ZC_STR_NOCASE);
}

/**
 * 把多个字符串结构用指定的字符串连接起来组成新的字符串结构, 
 * 必须为zcString类型，最后一个必须为NULL
 * @param sp  分隔的字符串
 * @return    成功返回新的字符串结构
 */
zcString*
zc_str_join (char *sp, ...)
{
    zcString    *newstr, *tmp;
    va_list     ap;
    int         slen = 0, lastlen = -1;

    if (NULL != sp)
        slen = strlen(sp);

    newstr = zc_str_new(2048);
    if (NULL == newstr){
        return NULL;
    }
   
    va_start(ap, sp);
    while (1) {
        tmp = va_arg(ap, zcString*);
        if (NULL == tmp)
            break;
        zc_str_append_len(newstr, tmp->data, tmp->len);
        lastlen = newstr->len;
        if (NULL != sp) {
            zc_str_append_len(newstr, sp, slen);
        }
    }
    va_end(ap);
    newstr->data[lastlen] = 0;
    newstr->len = lastlen;

    return newstr;
}

/**
 * 把多个C字符串和指定的字符串连接起来组成新的字符串结构
 * 每隔都是char*类型，最后一个必须为NULL
 * @param err 出错信息
 * @param sp  分隔字符串
 * @return 成功返回新字符串结构
 */
zcString*
zc_str_join_char (char *sp, ...)
{
    zcString     *newstr;
    char        *tmp;
    va_list     ap;
    int         slen = 0, lastlen = -1;

    if (NULL != sp)
        slen = strlen(sp);

    newstr = zc_str_new(2048);
    if (NULL == newstr){
        return NULL;
    }
   
    va_start(ap, sp);
    while (1) {
        tmp = va_arg(ap, char*);
        if (NULL == tmp)
            break;
        zc_str_append(newstr, tmp);
        lastlen = newstr->len;
        if (NULL != sp) {
            zc_str_append_len(newstr, sp, slen);
        }
    }
    va_end(ap);
    newstr->data[lastlen] = 0;
    newstr->len = lastlen;

    return newstr;
}

zcString*
zc_str_join_list(char *sp, zcList *list)
{
    zcString    *newstr, *tmp;
    int         slen = 0, lastlen = -1;

    if (NULL != sp)
        slen = strlen(sp);
    
    newstr = zc_str_new(2048);
    if (NULL == newstr)
        return NULL;

    zcListNode *node;
    zc_list_foreach(list,node) {
        tmp = (zcString*)node->data;
        zc_str_append_len(newstr, tmp->data, tmp->len);
        lastlen = newstr->len;
        if (sp != NULL) {
            zc_str_append_len(newstr, sp, slen);
        }
    }

    newstr->data[lastlen] = 0;
    newstr->len = lastlen;

    return newstr;
}

/**
 * 把字符串结构按特定字符串拆分为多个字符串
 * @param sstr    待拆分的字符串结构
 * @param smstr   分隔字符串
 * @param maxnum  最多几个切分
 * @return    成功返回字符串链表
 */

zcList*
zc_str_split(zcString* sstr, char *smstr, int maxnum)
{
    zcList *strlist = NULL;
    char    *start;
    int     slen, flen;
    int     i;
    char    *xx;
    int     block = 0;
    
    if (0 == sstr->len){
        return NULL;
    }
    start = sstr->data;
    slen = strlen(smstr);
    i = 0;
    while (i<sstr->len) {
        xx = strstr(start+i, smstr);
        if (NULL == xx) {
            flen = sstr->len - i;
        }else{
            flen = xx - (start + i);
        }
        if (flen >= 0) {
            //ZCINFO("block:%d, flen:%d, %s", block, flen, start+i);
            zcString *ss;
            if (flen == 0) {
                ss = zc_str_new(0);
            }else{
                ss = zc_str_new_char(start + i, flen);
            }
            if (NULL == strlist) {
                strlist = zc_list_new();
                if (NULL == strlist)
                    return NULL;
                strlist->del = zc_str_delete;
            }
            //ZCINFO("split:%d/%d %s\n", ss->len, ss->size, ss->data);
            //node = (zcSListNode*)zc_slisthead_new(zcSListNode);
            //node->data = ss;
            zc_list_append(strlist, ss);
            block++;

            if (xx && maxnum > 0 && block >= maxnum) {
                ss = zc_str_new_char(xx + slen, 0);
                //node = (zcSListNode*)zc_slisthead_new(zcSListNode);
                //node->data = ss;
                zc_list_append(strlist, ss);
                break;
            }
        }
        if (NULL == xx)
            break;
        i += flen + slen;
        //ZCINFO("i:%d, len:%d", i, sstr->len);
        if (i == sstr->len) {
            zc_list_append(strlist, zc_str_new(0));
        }
    }
    return strlist;
}


/**
 * 计算一个gb18030编码的字符串的长度
 * @param str 字符串结构指针
 */
int           
zc_str_wc_gb18030 (zcString *sstr)
{
    long   lRe = 0;
    //char   *pNow, *pSrc;
    char   *pSrc;
    unsigned char ch;

    ZC_STRING_CHECK(sstr);

    pSrc = sstr->data;
    //pNow = pSrc;
    while (*pSrc) {
		ch = *pSrc;
        //if ((ch >= 0x00) && (ch <= 0x7f)) // 单字节
        if (ch <= 0x7f) { // 单字节
            lRe++;
	    	pSrc++;
	    	continue;
        }else{
	    	pSrc++;
	    	ch = *pSrc;
	    	if ((ch >= 0x81) && (ch <= 0xfe)) { // 可能是双字节或者四字节
				if (((ch >= 0x40) && (ch <= 0x7e)) ||((ch >= 0x80) && (ch <= 0xfe))) {// 是双字节
		    		lRe++;
		    		pSrc++;
		    		continue;
				}else{
		    		if ((ch >= 0x30) && (ch <= 0x39)) { // 是四字节，第二字节
						pSrc++;
						ch = *pSrc;
		        		if ((ch >= 0x81) && (ch <= 0xfe)) { // 四字节第三字节
			    			pSrc++;
			    			ch = *pSrc;
			    			if ((ch >= 0x30) && (ch <= 0x39)) { // 四字节第四字节
								lRe++;
								pSrc++;
								continue;
			    			}else{
			        			// 第四字节错误
								return ZC_ERR_ENC;
			    			}
						}else{
			    			// 第三字节错误
			    			return ZC_ERR_ENC;
						}
		    		}else{
                        // 第二字节错误
						return ZC_ERR_ENC;
		    		}
				}
	    	}else{
				// 第一字节错误
				return ZC_ERR_ENC;
	    	}
		}
		pSrc++;
    }
    return lRe;
}

/**
 * 计算一个utf8编码的字符串的长度
 * @param str 字符串结构指针
 */
int           
zc_str_wc_utf8 (zcString *sstr)
{
    char    *ndata;
    int     i;
    unsigned char   ch;
    int     icount = 0;

    ZC_STRING_CHECK(sstr);
    ndata = sstr->data;
   
    i = 0;
    while(i < sstr->len) {
        ch = ndata[i];  
        if (ch >> 7 == 0) {
            i++;
        }else if (ch >> 5 == 6){
            i += 2; 
        }else if (ch >> 4 == 14){
            i += 3;
        }else if (ch >> 3 == 30){
            i += 4;
        }else if (ch >> 2 == 62){
            i+= 5;
        }else if (ch >> 1 == 126){
            i+= 6;
        }else{
            ZCWARN("Error!\n");
            return -1;
        }
        icount++; 
    }
    if (i > sstr->len)
        return -1;
    return  icount; 
}
/**
 * 计算一个big5编码的字符串的长度
 * @param str 字符串结构指针
 */
int           
zc_str_wc_big5 (zcString *sstr)
{
    ZC_STRING_CHECK(sstr);
    return sstr->len/2;
}

zcString*
zc_str_quote(zcString *sstr)
{
    zcString *s;
    int         i;

    s = zc_str_new(sstr->size + 10);  
    if (NULL == s)
        return NULL;
    
    zc_str_append_c(s, '"');
    for (i = 0; i<sstr->len; i++) {
        if (sstr->data[i] == '"') {
            zc_str_append_c(s, '\\');
        }
        zc_str_append_c(s, sstr->data[i]);
    }

    zc_str_append_c(s, '"');
    return s;
}

int
zc_str_append_escape(zcString *sstr, const char *input)
{
    const char *fromc = input;
    const char *s;
    const char *format = "\"\\/\b\f\n\r\t";
    
    //for (i = 0; i<sstr->len; i++) {
    while (*fromc) {
        s = format;
        while (*s != 0) {
            if (*fromc == *s) {
                switch(*s) {
                case '"':
                    zc_str_append(sstr, "\\\"");
                    break;
                case '\\':
                    zc_str_append(sstr, "\\\\");
                    break;
                case '/':
                    zc_str_append(sstr, "\\/");
                    break;
                case '\b':
                    zc_str_append(sstr, "\\b");
                    break;
                case '\f':
                    zc_str_append(sstr, "\\f");
                    break;
                case '\n':
                    zc_str_append(sstr, "\\n");
                    break;
                case '\r':
                    zc_str_append(sstr, "\\r");
                    break;
                case '\t':
                    zc_str_append(sstr, "\\t");
                    break;
                default:
                    zc_str_delete(sstr);
                    return ZC_ERR;
                }
                //continue;
                break;
            }else{
                s++;
                continue;
            }
        }
        if (*s == 0) {
            zc_str_append_c(sstr, *fromc);
        }
        fromc++;
    }
    return ZC_OK;
}


#ifdef ZOCLE_WITH_ICONV
#include <zocle/str/convert.h>

zcString*   
zc_str_convert(zcString *sstr, const char *fenc, const char *tenc)
{
    zcString *newstr;
    int       ret;

    newstr = zc_str_new(sstr->len * 2 + 1);
    //ret = zc_iconv_convert(fenc, tenc, newstr->data, newstr->size, sstr->data, sstr->len);
    ret = zc_iconv_convert(fenc, tenc, sstr->data, sstr->len, newstr->data, newstr->size);
    if (ret <= 0) {
        zc_str_delete(newstr);
        newstr = NULL;
    }
    newstr->len = ret;
    return newstr;
}
#else
zcString*   
zc_str_convert(zcString *sstr, const char *fenc, const char *tenc)
{
    return NULL;
} 
#endif


int zc_str_startswith (zcString *sstr, char *startstr)
{
    if (zc_str_find_str(sstr, 0, 0, startstr, ZC_STR_CASE) == 0)
        return ZC_TRUE;
    return ZC_FALSE;
}

int zc_str_startswith_case (zcString *sstr, char *startstr)
{
    if (zc_str_find_str(sstr, 0, 0, startstr, ZC_STR_NOCASE) == 0)
        return ZC_TRUE;
    return ZC_FALSE;
}


int zc_str_endswith (zcString *sstr, char *endstr)
{
    int len = strlen(endstr);
    if (len > sstr->len)
        return ZC_FALSE;

    if (strncmp(sstr->data+sstr->len-len, endstr, len) == 0) {
        return ZC_TRUE;
    }
    return ZC_FALSE;
}

int zc_str_endswith_case (zcString *sstr, char *endstr)
{
    int len = strlen(endstr);
    if (len > sstr->len)
        return ZC_FALSE;

    if (strncasecmp(sstr->data+sstr->len-len, endstr, len) == 0) {
        return ZC_TRUE;
    }
    return ZC_FALSE;
}


int	
zc_str_ensure_idle_size(zcString *sstr, int size)
{
    if (sstr->size - sstr->len < size) {
        //ZCINFO("ensure resize:%d, %d", sstr->size-sstr->len, size);
        int ret = zc_str_resize(sstr, sstr->size+size);
        if (ret < 0)
            return ret;
    }
    return ZC_OK;
}

