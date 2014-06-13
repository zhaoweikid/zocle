#include <zocle/str/cstrlist.h>
#include <string.h>
#include <zocle/mem/alloc.h>
#include <zocle/base/defines.h>
#include <stdarg.h>

zcCStrList* 
zc_cstrlist_new(int size, int n)
{
    zcCStrList *s = (zcCStrList*)zc_malloc(sizeof(zcCStrList));
    zc_cstrlist_init(s, size, n);
    return s;
}

void
zc_cstrlist_delete(void *x)
{
    zc_cstrlist_destroy(x);
    zc_free(x);
}

void
zc_cstrlist_init(zcCStrList *s, int size, int n)
{
    memset(s, 0, sizeof(zcCStrList));
    s->data = (char*)zc_malloc(size);
    memset(s->data, 0, size);
    s->size = size;
    s->maxn = n;
    int *v = (int*)s->data;
    v[0] = (s->maxn+1) * sizeof(int);
}

void
zc_cstrlist_init_stack(zcCStrList *s, char *buf, int size, int n)
{
    memset(s, 0, sizeof(zcCStrList));
    memset(buf, 0, size);
    s->data = buf;
    s->size = size;
    s->maxn = n;
    int *v = (int*)s->data;
    v[0] = (s->maxn+1) * sizeof(int);
}

void
zc_cstrlist_destroy(void *x)
{
    zcCStrList *s = (zcCStrList*)x;
    zc_free(s->data);
}

void		
zc_cstrlist_clear(zcCStrList *s)
{
    memset(s->data, 0, s->size);
    s->n   = 0;
}

const char*	
zc_cstrlist_get(zcCStrList *s, int pos)
{
    if (pos < 0)
        pos += s->n;
    if (pos < 0 || pos >= s->n)
        return NULL;
    int *v  = (int*)s->data;
    int idx = v[pos];
    return &s->data[idx];
}

int	
zc_cstrlist_append(zcCStrList *s, const char *str, int slen)
{
    if (s->n >= s->maxn)
        return ZC_ERR;
    if (slen <= 0)
        slen = strlen(str);
    int *v  = (int*)s->data;
    int idx = v[s->n];

    if (idx+slen+1 > s->size)
        return ZC_ERR;
    strncpy(s->data+idx, str, slen);    
    s->data[idx+slen] = 0;
    s->n++;
    v[s->n] = idx+slen+1;
    return s->n; 
}

int	
zc_cstrlist_append_format(zcCStrList *s, const char *format, ...)
{
    if (s->n >= s->maxn)
        return ZC_ERR;
    va_list arg;
    int done;
    int *v  = (int*)s->data;
    int idx = v[s->n];

    va_start(arg, format);
    done = vsnprintf(s->data+idx, s->size-idx, format, arg);
    s->data[idx+done] = 0;
    va_end(arg);

    s->n++;
    v[s->n] = idx+done+1;
    return done;
}


