#include <zocle/str/strbuf.h>
#include <zocle/base/defines.h>
#include <zocle/utils/funcs.h>
#include <zocle/log/logfile.h>
#include <zocle/mem/alloc.h>
#include <string.h>

zcStrBuf*   
zc_strbuf_new(zcString *s)
{
    zcStrBuf *buf;

    if (zc_strbuf_new2(&buf, s) != ZC_OK) {
        ZCFATAL("buffer create error!\n");
        return NULL;
    }
    return buf;
}

void
zc_strbuf_delete(void *b)
{
    zc_strbuf_destroy(b);
    zc_free(b);
}

int
zc_strbuf_new2(zcStrBuf **buf, zcString *s)
{
    zcStrBuf *buffer = (zcStrBuf*)zc_calloct(zcStrBuf);
    *buf = buffer;
    return zc_strbuf_init(buffer, s);
}

int zc_strbuf_init(zcStrBuf *buf, zcString *s)
{
    buf->next = NULL;
    buf->pos  = 0;
    if (s) {
        buf->data = s;
    }else{
        buf->data = zc_str_new(32);
    }

    return ZC_OK;
}

inline void
zc_strbuf_clear(zcStrBuf *buf)
{
    //memset(buf->data, 0, buf->size);
    buf->pos  = 0;
    if (buf->data) {
        zc_str_clear(buf->data);
    }
}

void
zc_strbuf_destroy(void *b)
{
    if (NULL == b) {
        ZCFATAL("zc_strbuf_destroy NULL pointer\n");
    }
}

inline int 
zc_strbuf_set(zcStrBuf *buf, void *data, int len)
{
    buf->pos = 0;
    return zc_str_assign(buf->data, data, len);
}

inline int
zc_strbuf_get(zcStrBuf *buf, void *data, int len)
{
    int leav = buf->data->len - buf->pos;
    if (len > leav)
        len = leav;
    memcpy(data, buf->data->data+buf->pos, len);
    buf->pos += len;
    if (buf->pos == buf->data->len) {
        buf->pos = 0;
        zc_str_clear(buf->data);
    }
    return len;
}

int 
zc_strbuf_append(zcStrBuf *buf, void *data, int len)
{
    return zc_str_append_len(buf->data, data, len);
}

int
zc_strbuf_compact(zcStrBuf *buf)
{
    if (buf->pos == 0)
        return ZC_OK;
    int leav = buf->data->len - buf->pos;
    if (leav > 0) {
        memmove(buf->data->data, buf->data->data + buf->pos, leav);
    }
    buf->data->data[leav] = 0;
    buf->data->len  = leav;
    buf->pos = 0;
    return ZC_OK;
}


