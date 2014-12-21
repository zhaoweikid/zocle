#include <zocle/str/buffer.h>
#include <zocle/base/defines.h>
#include <zocle/utils/funcs.h>
#include <zocle/log/logfile.h>
#include <zocle/mem/alloc.h>
#include <string.h>

zcBuffer* zc_buffer_resize_tw(zcBuffer *buf, uint32_t size);

zcBuffer*   
zc_buffer_new(uint32_t size)
{
    zcBuffer *buf;
    if (zc_buffer_new2(&buf, size) != ZC_OK) {
        ZCFATAL("buffer create error!\n");
        return NULL;
    }
    return buf;
}

zcBuffer*   
zc_buffer_new_ring(uint32_t size)
{
    zcBuffer *buf;
    if (zc_buffer_new2(&buf, size) != ZC_OK) {
        ZCFATAL("buffer create error!\n");
        return NULL;
    }
    buf->ring = 1;
    return buf;
}

void
zc_buffer_delete(void *b)
{
    zc_buffer_destroy(b);
    zc_free(b);
}

int
zc_buffer_new2(zcBuffer **buf, uint32_t size)
{
    zcBuffer *buffer;
    if (size == 0)
        size = 32;
    
    buffer = (zcBuffer*)zc_malloc(sizeof(zcBuffer) + size);
    *buf = buffer;
    return zc_buffer_init(buffer, size);
}

int zc_buffer_init(zcBuffer *buf, uint32_t size)
{
    buf->next = NULL;
    buf->size = size;
    buf->end  = 0;
    buf->pos  = 0;
    buf->ring = 0;

    return ZC_OK;
}

inline void
zc_buffer_clear(zcBuffer *buf)
{
    memset(buf->data, 0, buf->size);
    buf->end = 0;
    buf->pos = 0;
}

void
zc_buffer_destroy(void *b)
{
    if (NULL == b) {
        ZCFATAL("zc_buffer_destroy NULL pointer\n");
    }
}

int 
zc_buffer_set(zcBuffer *buf, void *data, int len)
{
    if (len > buf->size) {
        return ZC_ERR;
    }
    memcpy(buf->data, data, len);
    buf->end = len;
    buf->pos = 0;

    return ZC_OK;
}

// buffer use two-way
int
zc_buffer_get_tw(zcBuffer *buf, void *data, int len)
{
    int leav = zc_buffer_used(buf);
    if (len > leav)
        len = leav;

    if (buf->end > buf->pos) {
        memcpy(data, buf->data+buf->pos, len);
        buf->pos += len;
    }else{
        int len1 = buf->size - buf->pos;
        if (len1 >= len) {
            memcpy(data, buf->data+buf->pos, len);
            buf->pos += len;
            if (len1 == len)
                buf->pos = 0;
        }else{
            memcpy(data, buf->data+buf->pos, len1);
            memcpy(data+len1, buf->data, len-len1);
            buf->pos = len-len1;
        }
    }
    if (buf->pos == buf->end) {
        buf->pos = buf->end = 0;
    }

    return len;
}


int
zc_buffer_get(zcBuffer *buf, void *data, int len)
{
    if (buf->ring)
        return zc_buffer_get_tw(buf, data, len);

    int leav = buf->end - buf->pos;
    if (len > leav)
        len = leav;
    memcpy(data, buf->data+buf->pos, len);
    buf->pos += len;
    if (buf->pos == buf->end) {
        buf->pos = buf->end = 0;
    }
    return len;
}

int 
zc_buffer_append_tw(zcBuffer *buf, void *data, int len)
{
    if (zc_buffer_idle(buf) < len) {
        return ZC_ERR; 
    }
    
    if (buf->end > buf->pos) {
        int len1 = buf->size - buf->end;
        if (len1 >= len) {
            memcpy(&buf->data[buf->end], data, len);
            buf->end += len;
        }else{
            memcpy(&buf->data[buf->end], data, len1);
            memcpy(buf->data, data+len1, len-len1);
            buf->end = len-len1;
        }
    }else{
        memcpy(buf->data+buf->end, data, len);
        buf->end += len;
    }

    return ZC_OK;
}

int 
zc_buffer_append(zcBuffer *buf, void *data, int len)
{
    if (buf->ring)
        return zc_buffer_append_tw(buf, data, len);

    if (buf->end + len > buf->size) {
        return ZC_ERR; 
    }
    memcpy(buf->data+buf->end, data, len);
    buf->end += len;

    return ZC_OK;
}

zcBuffer*
zc_buffer_append_resize_tw(zcBuffer *buf, void *data, int len)
{
    int blen = zc_buffer_used(buf);
    int wantsize = blen+len;
    if (wantsize > buf->size) {
        int newsize = buf->size; 
        while (newsize < wantsize) {
            newsize = newsize * 2;
        }
        zcBuffer *newbuf = zc_buffer_resize_tw(buf, newsize);
        zc_buffer_delete(buf);  
        buf = newbuf;
    }
    memcpy(buf->data+buf->end, data, len);
    buf->end += len;

    return ZC_OK;
}



zcBuffer*
zc_buffer_append_resize(zcBuffer *buf, void *data, int len)
{
    if (buf->ring)
        return zc_buffer_append_resize_tw(buf, data, len);

    if (buf->end + len > buf->size) {
        int newsize = buf->size; 
        while (newsize < len+buf->end) {
            newsize = newsize * 2;
        }
        zcBuffer *newbuf = zc_buffer_resize(buf, newsize);
        zc_buffer_delete(buf);  
        buf = newbuf;
    }
    memcpy(buf->data + buf->end, data, len);
    buf->end += len;

    return ZC_OK;
}

int
zc_buffer_compact_tw(zcBuffer *buf)
{
    if (buf->pos == 0)
        return ZC_OK;
    int len = zc_buffer_used(buf);
    if (len == 0) {
        buf->pos = buf->end = 0;
        return ZC_OK;
    }

    char *tmp = (char*)zc_malloc(len);
    zc_buffer_get_tw(buf, tmp, len);
        
    memcpy(buf->data, tmp, len);
    buf->pos = 0;
    buf->end = len;
    zc_free(tmp);

    return ZC_OK;
}



int
zc_buffer_compact(zcBuffer *buf)
{
    if (buf->ring)
        return zc_buffer_compact_tw(buf);

    if (buf->pos == 0)
        return ZC_OK;
    int leav = buf->end - buf->pos;
    
    if (leav > 0) {
        memmove(buf->data, buf->data+buf->pos, leav);
    }
    buf->end = leav;
    buf->pos = 0;
    return ZC_OK;
}

zcBuffer*   
zc_buffer_resize_tw(zcBuffer *buf, uint32_t size)
{
    zcBuffer *newbuf = zc_buffer_new(size);  
    newbuf->end = zc_buffer_get_tw(buf, newbuf->data, newbuf->size);
    zc_buffer_delete(buf);

    return newbuf;
}


zcBuffer*   
zc_buffer_resize(zcBuffer *buf, uint32_t size)
{
    if (buf->ring)
        return zc_buffer_resize_tw(buf, size);
    zcBuffer *newbuf = zc_buffer_new(size);  
    newbuf->end = zc_buffer_get(buf, newbuf->data, newbuf->size);
    zc_buffer_delete(buf);

    return newbuf;
}


void 
zc_buffer_delete_all(void *x)
{
    zcBuffer *buf = (zcBuffer*)x;
    while (buf) {
        zcBuffer *buftmp = buf->next;
        zc_buffer_delete(buf);
        buf = buftmp;
    }
}

