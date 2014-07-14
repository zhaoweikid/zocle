#ifndef ZOCLE_STR_CSTRING_H
#define ZOCLE_STR_CSTRING_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <zocle/log/logfile.h>
#include <zocle/mem/alloc.h>
#include <zocle/str/string.h>

/*!
 安全的字符串结构
 */
typedef struct zc_cstring_t
{
    uint32_t	size;     // 分配的空间
    uint32_t    len;      // 字符串长度
    char        data[0];  // 字符串数据
}zcCString;

zcCString*  zc_cstr_new(int len);
void        zc_cstr_delete(void *); 
int			zc_cstr_init(zcCString *, int len);
void        zc_cstr_destroy(void *); 
zcCString*  zc_cstr_new_char(char *s, int len);
int         zc_cstr_new2(zcCString**, int len);
int			zc_cstr_format(zcCString *, char *format, ...);
void        zc_cstr_print(zcCString *);
int         zc_cstr_clear(zcCString *);
int         zc_cstr_zero(zcCString *);
int         zc_cstr_len(zcCString *);
int         zc_cstr_assign(zcCString *, const char *, int len);
int         zc_cstr_append(zcCString *, const char *);
int         zc_cstr_append_format(zcCString *, char *format, ...);
int         zc_cstr_append_len(zcCString *, const char *, int);
int         zc_cstr_append_c(zcCString *, const char );
int         zc_cstr_append_util(zcCString *, const char *, const char*);
int         zc_cstr_insert(zcCString *, int, const char *, int);
int         zc_cstr_truncate(zcCString *, int);
int		    zc_cstr_replace_str(zcCString *, zcCString *, const char* lookup, const char* newstr, 
							   int count, int icase);
int		    zc_cstr_replace(zcCString *, zcCString *, const char* lookup, const char* newstr, int count);
int		    zc_cstr_replace_case(zcCString *, zcCString *, const char* lookup, const char* newstr, int count);
int         zc_cstr_find_str(zcCString *, int from, int len, const char *cstr, int icase);
int         zc_cstr_find(zcCString *, const char *cstr);
int         zc_cstr_find_case(zcCString *, const char *cstr);
int			zc_cstr_remove_str(zcCString *, const char *cstr, int count, int icase);
int			zc_cstr_remove(zcCString *, const char *cstr);
int			zc_cstr_remove_case (zcCString *, const char *cstr);
int         zc_cstr_cap(zcCString *);
int         zc_cstr_low(zcCString *);
int         zc_cstr_reverse(zcCString *);
zcCString*  zc_cstr_dup(zcCString*);
int         zc_cstr_trim_(zcCString*, char ch, int direction);
int         zc_cstr_trim(zcCString*, char ch);
int         zc_cstr_ltrim(zcCString*, char ch);
int         zc_cstr_rtrim(zcCString*, char ch);
int         zc_cstr_strip_(zcCString*, char* chs, int direction);
int         zc_cstr_strip(zcCString*, char* chs);
int         zc_cstr_lstrip(zcCString*, char* chs);
int         zc_cstr_rstrip(zcCString*, char* chs);
int         zc_cstr_cmp_case(zcCString *, zcCString *, int len);
int         zc_cstr_cmp(zcCString *, zcCString *, int len);
int			zc_cstr_sub(zcCString *, zcCString *, int from, int len);
int         zc_cstr_sub_count_str(zcCString*, int from, int len, const char *lookup, int icase);
int         zc_cstr_sub_count(zcCString*, int from, int len, const char *lookup);
int         zc_cstr_sub_count_case(zcCString*, int from, int len, const char *lookup);
int			zc_cstr_join(zcCString *, char *sp, ...);
int			zc_cstr_join_char(zcCString *, char *sp, ...);
int			zc_cstr_join_list (zcCString *, char *sp, zcList *);
zcList*     zc_cstr_split(zcCString*, char *, int maxnum);
int         zc_cstr_wc_gb18030(zcCString *str);
int         zc_cstr_wc_utf8(zcCString *str);
int         zc_cstr_wc_big5(zcCString *str);
int			zc_cstr_quote(zcCString *sstr, zcCString *);
#ifdef ZOCLE_WITH_ICONV
int			zc_cstr_convert(zcCString *sstr, zcCString *, char *fenc, char *tenc);
#endif
/*int         zc_cstr_startswith(zcCString *sstr, char *startstr);
int         zc_cstr_startswith_case(zcCString *sstr, char *startstr);
int         zc_cstr_endswith(zcCString *sstr, char *endstr);
int         zc_cstr_endswith_case(zcCString *sstr, char *endstr);
*/
//#define     zc_cstr_print(s) zc_cstr_print_(s, __FILE__, __LINE__)
#define     zc_cstr_delete_null(s) do{zc_cstr_delete(s);s=NULL;}while(0)

#define     zc_cstr_alloc_stack(len) \
		(zcCString*)alloca(sizeof(zcCString)+len)

#define		ZC_CSTR_SIZE(sz)  sz/sizeof(zcCString)+2

#endif
