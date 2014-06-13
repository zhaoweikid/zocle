#include <zocle/ds/array.h>
#include <zocle/mem/alloc.h>
#include <zocle/base/defines.h>
#include <zocle/log/logfile.h>

#include <string.h>

zcArray*   
zc_array_new(uint32_t n)
{
    zcArray *v = zc_malloct(zcArray);
    zc_array_init(v, n);
    return v;
}

void
zc_array_delete(void *v)
{
    zc_array_destroy(v);
    zc_free(v);
}

int 
zc_array_init(zcArray *v, uint32_t n)
{
    memset(v, 0, sizeof(zcArray));
    
    if (n == 0) n = 32;
    v->data = (void**)zc_malloc(sizeof(void*)*n);
    memset(v->data, 0, sizeof(void*)*n); 
    v->capacity = n;

    return ZC_OK;
}

void
zc_array_destroy(void *x)
{
    zcArray *v = (zcArray*)x;
    if (v->del) {
        int i;
        for (i=0; i<v->len; i++) {
            if (v->data[i]) {
                v->del(v->data[i]);
            }
        }
    }
    zc_free(v->data);
    v->data = NULL;
}



void*   
zc_array_get(zcArray *v, uint32_t pos, void *defv)
{
    if (pos >= v->len) 
        return defv;
    
    void *x = v->data[pos];
    // FIXME: 0可能是正常值
    v->data[pos] = 0; 
    return x;
}

void*   
zc_array_at(zcArray *v, uint32_t pos, void *defv)
{
    if (pos >= v->len) 
        return defv;
    return v->data[pos];
}

int 
zc_array_set(zcArray *v, uint32_t pos, void *val)
{
    if (pos >= v->len) 
        return ZC_ERR;
    
    v->data[pos] = val; 
    return ZC_OK;
}

int 
zc_array_append(zcArray *v, void *val)
{
    if (v->len == v->capacity) {
        zc_array_resize(v, v->capacity*2);
    }

    v->data[v->len] = val;
    v->len++;
    return ZC_OK;
}

void*
zc_array_pop_back(zcArray *v, void *defv)
{
    if (v->len == 0)
        return defv;
    v->len--;
    return v->data[v->len];
}

int 
zc_array_resize(zcArray *v, uint32_t newsize)
{
    if (newsize <= v->capacity)
        return ZC_ERR;
    void *newdata = zc_calloc(sizeof(void*)*newsize);
    memcpy(newdata, v->data, v->len*sizeof(void*)); 
    zc_free(v->data);
    v->data = newdata;
    return ZC_OK;
}



