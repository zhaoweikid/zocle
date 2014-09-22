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
#include <zocle/str/cstring.h>

/**
 * 创建新的字符串结构
 * @param 初始化长度
 * @return     成功返回新的字符串结构
 */
zcCString*
zc_cstr_new (int len)
{
    zcCString *s;
    if (zc_cstr_new2(&s, len) != ZC_OK) {
        return NULL;
    }
    return s;
}

void
zc_cstr_delete(void *s)
{
    zc_cstr_destroy(s);
    zc_free(s);
}

/**
 * 创建新的字符串结构
 * @param step 字符串结构内部存储每次增长的大小
 * @param err  返回的出错信息
 * @return     成功返回新的字符串结构
 */
int
zc_cstr_new2 (zcCString **s, int len)
{
    zcCString  *sstr;
    
    len = (len > 0 ? len: 32);
    sstr = (zcCString *)zc_malloc(sizeof(zcCString) + len*sizeof(char));
    zc_cstr_init(sstr, len);

    *s = sstr;
    return ZC_OK;
}

/**
 * 创建新的字符串结构
 * @param s 待初始化的字符串
 * @param len 初始化长度
 * @return     成功返回新的字符串结构
 */
zcCString*   
zc_cstr_new_char(char *str, int len)
{
    zcCString *s;
    
    if (NULL == str)
        return NULL;

    if (len <= 0) {
        len = strlen(str);
    }
    s = zc_cstr_new(len + 1);
    memcpy(s->data, str, len);
    s->data[len] = 0;
    s->len = len;
    
    return s;
}

int
zc_cstr_init(zcCString *s, int len)
{
    s->data[0] = 0;
    s->size = len;
    s->len = 0;
    return ZC_OK;
}

/**
 * @brief 释放字符串结构中占用的资源
 * @param sstr 字符串结构指针
 */
void
zc_cstr_destroy(void *s)
{
    if (NULL == s) {
        ZCERROR("zc_cstr_destroy NULL pointer!\n");
    }
}

/**
 * @brief 输出字符串的内容和属性
 * @param sstr    字符串结构指针
 */
void
zc_cstr_print(zcCString *sstr)
{
    if (NULL == sstr) {
        ZCINFO("cstring is NULL. %p\n", sstr);
    }else{
        ZCINFO("cstring size:%d len:%d, data:%p, %s\n", 
                sstr->size, sstr->len, sstr->data, sstr->data);
    }
}


/**
 * 清除字符串结构内部数据
 * @param sstr 字符串结构指针
 * @return    正常返回ZC_OK
 */
int
zc_cstr_clear (zcCString *sstr)
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
zc_cstr_zero (zcCString *sstr)
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
zc_cstr_len (zcCString *sstr)
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
zc_cstr_assign (zcCString *sstr, const char *ccstr, int cslen)
{
    if (ccstr == NULL) {
        return ZC_ERR_NULL;
    }

    if (cslen <= 0)
        cslen = strlen(ccstr);
    
    if (cslen + 1 > sstr->size) {
        return ZC_ERR;
    }
    memcpy(sstr->data, ccstr, cslen);
    sstr->data[cslen] = 0;
    sstr->len = cslen;
    return ZC_OK;
}

/**
 * 把格式化字符串存储
 */

int
zc_cstr_format(zcCString *s, char *format, ...)
{
    va_list arg;
    int     done;
    
    va_start(arg, format);
    done = vsnprintf(s->data, s->size, format, arg);
    s->data[done] = 0;
    s->len = done;
    /*ret = zc_cstr_append_len(s, buf, done);
    if (ret < 0) {
        return ret;
    }*/
    va_end(arg);

    return ZC_OK;
}


/**
 * 向字符串结构中追加数据
 * @param sstr    字符串结构指针
 * @param ccstr   C字符串指针
 * @return    成功返回ZC_OK
 */
int
zc_cstr_append (zcCString *sstr, const char *ccstr)
{
    int     isize, cslen; 
    char    *copyto;

    if (ccstr == NULL) {
        return ZC_ERR_NULL;
    }
    
    cslen = strlen(ccstr);
    isize = sstr->len + cslen + 1;
  
    if (isize > sstr->size) {
        return ZC_ERR;
    }
    
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
zc_cstr_append_format(zcCString *sstr, char *format, ...)
{
    va_list arg;
    char    buf[8192];
    int     maxlen = 8191;
    int     done;
    int     ret;
    
    va_start(arg, format);
    done = vsnprintf(buf, maxlen, format, arg);
    buf[done] = 0;
    ret = zc_cstr_append_len(sstr, buf, done);
    if (ret < 0) {
        return ret;
    }
    va_end(arg);

    return ZC_OK;
}

/**
 * 向字符串结构中追加数据
 * @param sstr    字符串结构指针
 * @param ccstr   C字符串指针
 * @param cslen   C字符串长度
 * @return    成功返回ZC_OK
 */
int
zc_cstr_append_len (zcCString *sstr, const char *ccstr, int cslen)
{
    int     isize; 
    char    *copyto;

    if (ccstr == NULL) {
        return ZC_ERR_NULL;
    }
    
    if (cslen <= 0)
        cslen = strlen(ccstr);
    isize = sstr->len + cslen + 1;
  
    if (isize > sstr->size) {
        return ZC_ERR;
    }
    
    copyto = sstr->data + sstr->len;
    memcpy(copyto, ccstr, cslen);
    sstr->len = isize - 1;
    sstr->data[sstr->len] = 0;

    return ZC_OK;
}


/**
 * 向字符串结构中追加数据
 * @param sstr    字符串结构指针
 * @param ccstr   C字符串指针
 * @return    成功返回ZC_OK
 */
int
zc_cstr_append_c (zcCString *sstr, const char c)
{
    int len = sstr->len;
    if (len + 2 > sstr->size) {
        return ZC_ERR;
    }
    sstr->data[len] = c;
    sstr->data[len+1] = 0;
    sstr->len++;
    
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
zc_cstr_insert (zcCString *sstr, int pos, const char *ccstr, int cslen)
{
    int     isize; 

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
    if (isize > sstr->size) {
        return ZC_ERR;
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
zc_cstr_truncate (zcCString *sstr, int pos)
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
int
zc_cstr_replace_str(zcCString *sstr, zcCString *newsstr, const char* lookup, const char* newstr, 
                    int count, int icase)
{
    int     currep = 0;
    int     looklen, newstrlen = 0;
    int     found = ZC_FALSE;
    char    *cdata, *rets, *end;
    int     ret;

    if (sstr->len == 0 || NULL == lookup)
        return ZC_ERR_NULL;
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
            if (ZC_TRUE == found){ // copy string end block
                ret = zc_cstr_append_len(newsstr, cdata, (sstr->data+sstr->len)-cdata);
                if (ret < 0) {
                    return ZC_ERR;
                }
            }else{
                //newsstr = zc_cstr_copy(newsstr, sstr);
                ret = zc_cstr_append_len(newsstr, sstr->data, sstr->len);
                if (ret < 0)
                    return ZC_ERR;
            }
            break;
        }else{
            if (ZC_FALSE == found) { //first, create new dest string
                //newsstr = zc_cstr_new_char(sstr->data, rets - sstr->data);
                ret = zc_cstr_append_len(newsstr, sstr->data, rets-sstr->data);
                if (ret < 0)
                    return ZC_ERR;
                found = ZC_TRUE;
            }
            if (newstrlen > 0) {
                ret = zc_cstr_append_len(newsstr, newstr, newstrlen);
                if (ret < 0)
                    return ZC_ERR;
            }
            cdata = rets + looklen;
            currep++;

            if (count > 0 && currep == count) {
                ret = zc_cstr_append_len(newsstr, cdata, end-cdata);
                if (ret < 0)
                    return ZC_ERR;
                break;
            }
        }
    }
    return ZC_OK;
}

int
zc_cstr_replace(zcCString *sstr, zcCString *s, const char* lookup, const char* newstr, int count)
{
    return zc_cstr_replace_str(sstr, s, lookup, newstr, count, ZC_STR_CASE);
}

int
zc_cstr_replace_case(zcCString *sstr, zcCString *s, const char* lookup, const char* newstr, int count)
{
    return zc_cstr_replace_str(sstr, s, lookup, newstr, count, ZC_STR_NOCASE);
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
zc_cstr_find_str (zcCString *sstr, int from, int len, const char *cstr, int icase)
{
    int     looklen;
    char    *cdata, *ret, *end;

    if (len < 0)
        return ZC_ERR_PARAM;

    looklen = strlen(cstr);
   
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
zc_cstr_find (zcCString *sstr, const char *cstr)
{
    return zc_cstr_find_str(sstr, 0, 0, cstr, ZC_STR_CASE);
}

int
zc_cstr_find_case (zcCString *sstr, const char *cstr)
{
    return zc_cstr_find_str(sstr, 0, 0, cstr, ZC_STR_NOCASE);
}


/**
 * 从字符串结构中删除指定字符串
 * @param sstr    字符串结构指针
 * @param cstr    查找的字符串
 * @param count   匹配次数
 * @param icase   是否忽略大小写，1为不区分，0为区分
 * @return        成功返回ZC_OK
 */
int
zc_cstr_remove_str (zcCString *sstr, const char *lookup, int count, int icase)
{
    int     currep = 0;
    int     looklen;
    char    *cdata, *rets, *end;

    if (sstr->len == 0 || NULL == lookup)
        return ZC_ERR_NULL;
    looklen = strlen(lookup);
    cdata = sstr->data;
    end   = sstr->data + sstr->len;

    while (1) {
        if (ZC_STR_NOCASE == icase){
            rets = strcasestr(cdata, lookup);
        }else{
            rets = strstr(cdata, lookup);
        }
        if (rets == NULL)
            break;
        
        memmove(rets, rets+looklen, end-rets); 
        sstr->len -= looklen;
        cdata = rets;
        currep++;
        if (count > 0 && currep == count) {
            break;
        }
    }
    return ZC_OK;
}

int
zc_cstr_remove (zcCString *sstr, const char *cstr)
{
    return zc_cstr_remove_str(sstr, cstr, 0, ZC_STR_CASE);    
}

int
zc_cstr_remove_case (zcCString *sstr, const char *cstr)
{
    return zc_cstr_remove_str(sstr, cstr, 0, ZC_STR_NOCASE);    
}

/**
 * 把字符串结构中的字符串改为大写
 * @param sstr    字符串结构指针
 * @return 成功返回ZC_OK
 */
int           
zc_cstr_cap (zcCString *sstr)
{
    char    *cdata;
    int     i;

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
zc_cstr_low (zcCString *sstr)
{
    char    *cdata;
    int     i;

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
zc_cstr_reverse (zcCString *sstr)
{
    char   ch;
    char   *now;
    char   *src;
    
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
zcCString*
zc_cstr_dup(zcCString*  sstr)
{
    zcCString  *newstr;

    newstr = zc_cstr_new(sstr->size);
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
zc_cstr_trim_(zcCString* sstr, char ch, int direction)
{
    char    *cdata;
    int     i;

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
zc_cstr_trim(zcCString* sstr, char ch)
{
    return zc_cstr_trim_(sstr, ch, ZC_STR_BOTH);
}

int 
zc_cstr_ltrim(zcCString *sstr, char ch)
{
    return zc_cstr_trim_(sstr, ch, ZC_STR_LEFT);
}

int 
zc_cstr_rtrim(zcCString *sstr, char ch)
{
    return zc_cstr_trim_(sstr, ch, ZC_STR_RIGHT);
}

/**
 * 去掉字符串结构中的指定字符
 * @param sstr    字符串结构指针
 * @param chs     带去掉的所有字符组成的字符串
 * @param direction 需要去掉的位置，小于0去掉头部的，大于0去掉尾部的，等于0头尾都去
 * @return    成功返回ZC_OK
 */
int
zc_cstr_strip_(zcCString *sstr, char* chs, int direction)
{
    char    *chs_default = " \t\r\n";
    char    *cdata;
    int     i, k;
    int     clen;

    if (NULL == chs)
        chs = chs_default;
    
    clen = strlen(chs);
    cdata = sstr->data;

    if (direction <= ZC_STR_BOTH) { // left
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
    
    if (direction >= ZC_STR_BOTH) { // right
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
zc_cstr_strip(zcCString *sstr, char* chs)
{
    return zc_cstr_strip_(sstr, chs, ZC_STR_BOTH);
}

int
zc_cstr_lstrip(zcCString *sstr, char* chs)
{
    return zc_cstr_strip_(sstr, chs, ZC_STR_LEFT);
}

int
zc_cstr_rstrip(zcCString *sstr, char* chs)
{
    return zc_cstr_strip_(sstr, chs, ZC_STR_RIGHT);
}

/**
 * 字符串结构的比较
 * @param lsstr   待比较的字符串结构
 * @param rsstr   待比较的字符串结构
 * @param len     比较的长度
 * @return        相等返回0，行为和strcmp一样
 */
int           
zc_cstr_cmp_case (zcCString *lsstr, zcCString *rsstr, int len)
{
    if (len > 0)
        return strncasecmp(lsstr->data, rsstr->data, len);
    else
        return strcasecmp(lsstr->data, rsstr->data);
}

int           
zc_cstr_cmp (zcCString *lsstr, zcCString *rsstr, int len)
{
    if (len > 0)
        return strncmp(lsstr->data, rsstr->data, len);
    else
        return strcmp(lsstr->data, rsstr->data);
}


/**
 * 求子字符串
 * @param sstr    字符串结构
 * @param from   子字符串的开始位置
 * @param len   子字符串的长度
 * @return    成功返回新字符串结构
 */
int
zc_cstr_sub (zcCString *sstr, zcCString *newstr, int from, int len)
{
    char        *cdata;

    if (0 == sstr->len || len == 0){
        return ZC_ERR;
    }
    if (from < 0) {
        from = sstr->len + from;
    }
    if (from >= sstr->len)
        return ZC_ERR;

    cdata = sstr->data + from;
    int ret = zc_cstr_assign(newstr, cdata, len);
    if (ret < 0)
        return ZC_ERR;
    return ZC_OK;
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
zc_cstr_sub_count_str(zcCString *sstr, int from, int len, const char* lookup, int icase)
{
    int     looklen;
    char    *cdata, *ret, *end;
    int     icount = 0;

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
zc_cstr_sub_count(zcCString *sstr, int from, int len, const char* lookup)
{
    return zc_cstr_sub_count_str(sstr, from, len, lookup, ZC_STR_CASE);
}

int
zc_cstr_sub_count_case(zcCString *sstr, int from, int len, const char* lookup)
{
    return zc_cstr_sub_count_str(sstr, from, len, lookup, ZC_STR_NOCASE);
}


/**
 * 把多个字符串结构用指定的字符串连接起来组成新的字符串结构, 
 * 必须为zcCString类型，最后一个必须为NULL
 * @param sp  分隔的字符串
 * @return    成功返回新的字符串结构
 */
int
zc_cstr_join (zcCString *newstr, char *sp, ...)
{
    zcCString   *tmp;
    va_list     ap;
    int         slen = 0, lastlen = -1;
    int         ret;

    if (NULL != sp)
        slen = strlen(sp);

    va_start(ap, sp);
    while (1) {
        tmp = va_arg(ap, zcCString*);
        if (NULL == tmp)
            break;
        if (zc_cstr_append_len(newstr, tmp->data, tmp->len) != ZC_OK) {
            return ZC_ERR;
        }
        lastlen = newstr->len;
        if (NULL != sp) {
            ret = zc_cstr_append_len(newstr, sp, slen);
            if (ret < 0)
                return ZC_ERR;
        }
    }
    va_end(ap);
    newstr->data[lastlen] = 0;
    newstr->len = lastlen;

    return ZC_OK;
}

/**
 * 把多个C字符串和指定的字符串连接起来组成新的字符串结构
 * 每隔都是char*类型，最后一个必须为NULL
 * @param err 出错信息
 * @param sp  分隔字符串
 * @return 成功返回新字符串结构
 */
int
zc_cstr_join_char (zcCString *newstr, char *sp, ...)
{
    char    *tmp;
    va_list ap;
    int     slen = 0, lastlen = -1;
    int     ret;

    if (NULL != sp)
        slen = strlen(sp);

    va_start(ap, sp);
    while (1) {
        tmp = va_arg(ap, char*);
        if (NULL == tmp)
            break;
        ret = zc_cstr_append(newstr, tmp);
        if (ret < 0)
            return ZC_ERR;
        lastlen = newstr->len;
        if (NULL != sp) {
            ret = zc_cstr_append_len(newstr, sp, slen);
            if (ret < 0)
                return ZC_ERR;
        }
    }
    va_end(ap);
    newstr->data[lastlen] = 0;
    newstr->len = lastlen;

    return ZC_OK;
}

int
zc_cstr_join_list(zcCString *newstr, char *sp, zcList *list)
{
    zcCString   *tmp;
    int slen = 0, lastlen = -1, ret;

    if (NULL != sp)
        slen = strlen(sp);
   
    zcListNode *node;
    zc_list_foreach(list,node) {
        tmp = (zcCString*)node->data;
        ret = zc_cstr_append_len(newstr, tmp->data, tmp->len);
        if (ret < 0)
            return ZC_ERR;
        lastlen = newstr->len;
        if (sp != NULL) {
            ret = zc_cstr_append_len(newstr, sp, slen);
            if (ret < 0)
                return ZC_ERR;
        }
    }
    newstr->data[lastlen] = 0;
    newstr->len = lastlen;

    return ZC_OK;
}

/**
 * 把字符串结构按特定字符串拆分为多个字符串
 * @param sstr    待拆分的字符串结构
 * @param smstr   分隔字符串
 * @param maxnum  最多几个切分
 * @return    成功返回字符串链表
 */

zcList*
zc_cstr_split(zcCString* sstr, char *smstr, int maxnum)
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
    while (1) {
        xx = strstr(start+i, smstr);
        if (NULL == xx) {
            flen = sstr->len - i;
        }else{
            flen = xx - (start + i);
        }
        if (flen >= 0) {
            zcCString *ss;

            if (flen == 0) {
                ss = zc_cstr_new(0);
            }else{
                ss = zc_cstr_new_char(start + i, flen);
            }
            if (NULL == strlist) {
                strlist = zc_list_new();
                if (NULL == strlist)
                    return NULL;
                strlist->del = zc_cstr_delete;
            }
            //ZCINFO("split: %s\n", ss->data);
            zc_list_append(strlist, ss);
            block++;

            if (xx && maxnum > 0 && block >= maxnum) {
                ss = zc_cstr_new_char(xx + slen, 0);
                zc_list_append(strlist, ss);
                break;
            }
        }
        if (NULL == xx)
            break;
        i += flen + slen;
    }

    return strlist;
}


/**
 * 计算一个gb18030编码的字符串的长度
 * @param str 字符串结构指针
 */
int           
zc_cstr_wc_gb18030 (zcCString *sstr)
{
    long   ret = 0;
    char   *psrc;
    unsigned char ch;

    psrc = sstr->data;
    //char *pnow = psrc;
    while (*psrc) {
		ch = *psrc;
        //if ((ch >= 0x00) && (ch <= 0x7f)) // 单字节
        if (ch <= 0x7f) { // 单字节
            ret++;
	    	psrc++;
	    	continue;
        }else{
	    	psrc++;
	    	ch = *psrc;
	    	if ((ch >= 0x81) && (ch <= 0xfe)) { // 可能是双字节或者四字节
				if (((ch >= 0x40) && (ch <= 0x7e)) ||((ch >= 0x80) && (ch <= 0xfe))) {// 是双字节
		    		ret++;
		    		psrc++;
		    		continue;
				}else{
		    		if ((ch >= 0x30) && (ch <= 0x39)) { // 是四字节，第二字节
						psrc++;
						ch = *psrc;
		        		if ((ch >= 0x81) && (ch <= 0xfe)) { // 四字节第三字节
			    			psrc++;
			    			ch = *psrc;
			    			if ((ch >= 0x30) && (ch <= 0x39)) { // 四字节第四字节
								ret++;
								psrc++;
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
		psrc++;
    }
    return ret;
}

/**
 * 计算一个utf8编码的字符串的长度
 * @param str 字符串结构指针
 */
int           
zc_cstr_wc_utf8 (zcCString *sstr)
{
    char    *ndata;
    int     i;
    unsigned char   ch;
    int     icount = 0;

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
        }
        icount++; 
    }
    return  icount; 
}
/**
 * 计算一个big5编码的字符串的长度
 * @param str 字符串结构指针
 */
int           
zc_cstr_wc_big5 (zcCString *sstr)
{
    return sstr->len/2;
}

int
zc_cstr_quote(zcCString *sstr, zcCString *s)
{
    int i;

    int ret = zc_cstr_append_c(s, '"');
    if (ret < 0)
        return ZC_ERR;
    for (i = 0; i<sstr->len; i++) {
        if (sstr->data[i] == '"') {
            ret = zc_cstr_append_c(s, '\\');
            if (ret < 0)
                return ZC_ERR;
        }
        zc_cstr_append_c(s, sstr->data[i]);
    }

    zc_cstr_append_c(s, '"');
    return ZC_OK;
}

#ifdef ZOCLE_WITH_ICONV
#include <zocle/str/convert.h>

int   
zc_cstr_convert(zcCString *sstr, zcCString *newstr, char *fenc, char *tenc)
{
    //zcCString  *newstr;
    int         ret;

    ret = zc_iconv_convert(fenc, tenc, newstr->data, newstr->size, sstr->data, sstr->len);
    if (ret <= 0) {
        return ZC_ERR;
    }

    return ZC_OK;
}
#else
int   
zc_cstr_convert(zcCString *sstr, zcCString *newstr, char *fenc, char *tenc)
{
    return zc_cstr_append_len(newstr, sstr->data, sstr->len);
}
#endif


/*int zc_cstr_startswith (zcCString *sstr, char *startstr)
{
    if (zc_cstr_find_str(sstr, 0, 0, startstr, ZC_STR_CASE) == 0)
        return ZC_TRUE;
    return ZC_FALSE;
}

int zc_cstr_startswith_case (zcCString *sstr, char *startstr)
{
    if (zc_cstr_find_str(sstr, 0, 0, startstr, ZC_STR_NOCASE) == 0)
        return ZC_TRUE;
    return ZC_FALSE;
}


int zc_cstr_endswith (zcCString *sstr, char *endstr)
{
    int len = strlen(endstr);
    if (len > sstr->len)
        return ZC_FALSE;

    if (strncmp(sstr->data+sstr->len-len, endstr, len) == 0) {
        return ZC_TRUE;
    }
    return ZC_FALSE;
}

int zc_cstr_endswith_case (zcCString *sstr, char *endstr)
{
    //int len = strlen(endstr);
    //if (len > sstr->len)
    //    return ZC_FALSE;

    if (strncasecmp(sstr->data+sstr->len-len, endstr, len) == 0) {
        return ZC_TRUE;
    }
    return ZC_FALSE;
}*/


