#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <zocle/zocle.h>

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);
    
    zcBuffer *buf;

    buf = zc_buffer_new(4096);
    assert(buf->size == 4096);

    char *s = "abcdefg";
    int  slen = strlen(s);

    zc_buffer_set(buf, s, slen);
    assert(buf->size == 4096);
    assert(buf->end == slen);
    assert(buf->pos == 0);

    char xbuf[1024] = {0};
    int  ret;

    ret = zc_buffer_get(buf, xbuf, 100);
    assert(ret == slen);
    assert(strcmp(xbuf, s) == 0);
    assert(buf->pos == buf->pos);
    assert(buf->pos == slen);

    zc_buffer_compact(buf);
    assert(buf->pos == 0);
    assert(buf->end == 0);


    zc_buffer_append(buf, s, slen);
    assert(strncmp(buf->data, s, slen) == 0);
    assert(buf->end == slen);
    
    char ns[128];
    sprintf(ns, "%shah", s);
    int  nslen = strlen(ns);

    zc_buffer_append(buf, "hah", 3);
    assert(buf->end == nslen);
    assert(strncmp(buf->data, ns, nslen) == 0);

    
    zc_buffer_delete(buf);

    return 0;
}
