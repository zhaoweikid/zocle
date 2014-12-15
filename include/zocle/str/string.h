#ifndef ZOCLE_STR_STRING_H
#define ZOCLE_STR_STRING_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <zocle/log/logfile.h>
#include <zocle/ds/list.h>
#include <zocle/mem/alloc.h>
#include <zocle/base/type.h>

#define ZC_STR_LEFT  -1
#define ZC_STR_BOTH	 0
#define ZC_STR_RIGHT 1

#define ZC_STR_CASE		1
#define ZC_STR_NOCASE	0

char*   zc_strdup(const char* s, int len);
char*   zc_strnstr(const char*, const char*, int);
char*   zc_strnchr(const char*, const char, int);

/*!
 安全的字符串结构
 */
typedef struct zc_string_t
{
	ZC_OBJECT_HEAD
    uint32_t size;     // 分配的空间
	uint8_t	 _onstack:1;
    uint32_t len:31;      // 字符串长度
    char     *data;    // 字符串数据
}zcString;

zcString*   zc_str_new(int len);
void        zc_str_delete(void *); 
void        zc_str_delete3(void *, const char *file, int lineno); 
int			zc_str_init(zcString *, int len);
void	    zc_str_init_stack(zcString *s, char *buf, int len);
void	    zc_str_init_stack_exist(zcString *s, char *buf, int len);
void        zc_str_destroy(void *); 
void        zc_str_destroy3(void *, const char *file, int lineno); 
zcString*   zc_str_new_char(char *s, int len);
int         zc_str_new2(zcString**, int len);
zcString*   zc_str_format(zcString *, char *format, ...);
void        zc_str_print(zcString *);
void        zc_str_printr(zcString *);
int         zc_str_clear(zcString *);
int         zc_str_zero(zcString *);
int         zc_str_len(zcString *);
zcString*   zc_str_copy(zcString *, zcString*);
int         zc_str_assign(zcString *, const char *, int len);
int         zc_str_append(zcString *, const char *);
int         zc_str_append_format(zcString *, char *format, ...);
int         zc_str_append_escape(zcString *, const char *format);
int         zc_str_append_len(zcString *, const char *, int);
int         zc_str_append_until(zcString *, const char *, const char*);
zcString*   zc_str_append_len_new(zcString *sstr, const char *ccstr, int cslen);
int         zc_str_append_c(zcString *, const char );
int         zc_str_append_util(zcString *, const char *, const char*);
int         zc_str_skip_len(zcString *, int);
int         zc_str_insert(zcString *, int, const char *, int);
int         zc_str_truncate(zcString *, int);
zcString*   zc_str_replace_str(zcString *, const char* lookup, const char* newstr, 
							   int count, int icase);
zcString*   zc_str_replace(zcString *, const char* lookup, const char* newstr, int count);
zcString*   zc_str_replace_case(zcString *, const char* lookup, const char* newstr, int count);
int         zc_str_find_str(zcString *, int from, int len, const char *cstr, int icase);
int         zc_str_find(zcString *, const char *cstr);
int         zc_str_find_case(zcString *, const char *cstr);
zcString*   zc_str_remove_str(zcString *, const char *cstr, int count, int icase);
zcString*   zc_str_remove(zcString *, const char *cstr);
zcString*   zc_str_remove_case (zcString *, const char *cstr);
int         zc_str_cap(zcString *);
int         zc_str_low(zcString *);
int         zc_str_reverse(zcString *);
zcString*   zc_str_dup(zcString*);
int         zc_str_trim_(zcString*, char ch, int direction);
int         zc_str_trim(zcString*, char ch);
int         zc_str_ltrim(zcString*, char ch);
int         zc_str_rtrim(zcString*, char ch);
int         zc_str_strip_(zcString*, char* chs, int direction);
int         zc_str_strip(zcString*, char* chs);
int         zc_str_lstrip(zcString*, char* chs);
int         zc_str_rstrip(zcString*, char* chs);
int         zc_str_cmp(zcString *, zcString *, int len);
int         zc_str_cmp_case(zcString *, zcString *, int len);
zcString*   zc_str_sub(zcString *, int from, int len);
int         zc_str_sub_count_str(zcString*, int from, int len, const char *lookup, int icase);
int         zc_str_sub_count(zcString*, int from, int len, const char *lookup);
int         zc_str_sub_count_case(zcString*, int from, int len, const char *lookup);
zcString*   zc_str_join(char *sp, ...);
zcString*   zc_str_join_char(char *sp, ...);
zcString*   zc_str_join_list (char *sp, zcList *);
zcList*     zc_str_split(zcString*, char *, int maxnum);
int         zc_str_wc_gb18030(zcString *str);
int         zc_str_wc_utf8(zcString *str);
int         zc_str_wc_big5(zcString *str);
zcString*   zc_str_quote(zcString *sstr);

#ifdef ZOCLE_WITH_ICONV
zcString*   zc_str_convert(zcString *sstr, const char *fenc, const char *tenc);
#endif
int         zc_str_startswith(zcString *sstr, char *startstr);
int         zc_str_startswith_case(zcString *sstr, char *startstr);
int         zc_str_endswith(zcString *sstr, char *endstr);
int         zc_str_endswith_case(zcString *sstr, char *endstr);

int			zc_str_ensure_idle_size(zcString *sstr, int size);

#define     zc_str_idle(s)     (s->size-s->len)
#define     zc_str_delete_null(s) do{zc_str_delete(s);s=NULL;}while(0)

#define		zc_str_alloc_stack() (zcString*)alloca(sizeof(zcString))

#endif
