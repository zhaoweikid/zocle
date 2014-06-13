#ifndef ZOCLE_STR_BUFFER_H
#define ZOCLE_STR_BUFFER_H

#include <stdio.h>
#include <stdint.h>
#include <zocle/ds/listhead.h>

typedef struct zc_buffer_t
{
    struct zc_buffer_t *next;
    uint32_t size;
    uint32_t end;
    uint32_t pos;
    char     ring;
    char     data[0];
}zcBuffer;

zcBuffer*   zc_buffer_new(uint32_t size);
void        zc_buffer_delete(void *);
int         zc_buffer_new2(zcBuffer **, uint32_t size);
int			zc_buffer_init(zcBuffer*, uint32_t size);
void        zc_buffer_destroy(void *);
void        zc_buffer_clear(zcBuffer *);

int         zc_buffer_set(zcBuffer *, void *data, int len);
int         zc_buffer_get(zcBuffer *, void *data, int len);
int         zc_buffer_append(zcBuffer *, void *data, int len);
zcBuffer*   zc_buffer_append_resize(zcBuffer *, void *data, int len);
int			zc_buffer_compact(zcBuffer*);
zcBuffer*   zc_buffer_resize(zcBuffer *, uint32_t size);


#define		zc_buffer_alloc_stack(size) (zcBuffer*)alloca(sizeof(zcBuffer)+size)
#define     zc_buffer_reset(buf)	buf->pos=buf->end=0
//#define		zc_buffer_len(buf)		buf->end-buf->pos
//#define     zc_buffer_idle(buf)		buf->size-buf->end
#define		zc_buffer_data(buf)		&buf->data[buf->pos]
#define     zc_buffer_delete_null(x) do{zc_buffer_delete(x);x=NULL;}while(0)

inline int
zc_buffer_len(zcBuffer *buf)
{
    if (buf->ring) {
        return buf->end>=buf->pos? buf->end-buf->pos: buf->size-buf->pos+buf->end;
    }
    return buf->end - buf->pos;
}

inline int
zc_buffer_idle(zcBuffer *buf)
{
    if (buf->ring) {
        return buf->end-buf->pos? buf->size-buf->end+buf->pos: buf->pos-buf->end;
    }
    return buf->size - buf->end;
}



#endif
