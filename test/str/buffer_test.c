#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <zocle/zocle.h>

int test()
{
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

    int copyn = 5;
    ret = zc_buffer_get(buf, xbuf, copyn);
    assert(ret == copyn);
    assert(strncmp(xbuf, s, copyn) == 0);
    assert(buf->pos == copyn);
    //assert(buf->pos == 0);

    zc_buffer_compact(buf);
    assert(buf->pos == 0);
    assert(buf->end == slen-copyn);

    int newlen = slen - copyn + slen;
    zc_buffer_append(buf, s, slen);
    assert(strncmp(buf->data+(slen-copyn), s, slen) == 0);
    assert(buf->end == newlen);
    
    char ns[128] = {0};
    strncpy(ns, buf->data, newlen);
    //strcat(ns, s);
    strcat(ns, "hah");
    //sprintf(ns, "%shah", s);
    int  nslen = strlen(ns);

    zc_buffer_append(buf, "hah", 3);
    assert(buf->end == nslen);
    assert(strncmp(buf->data, ns, nslen) == 0);
    
    zc_buffer_delete(buf);

    return 0;
}

int test_ring()
{

    return 0;
}

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);

    test();
    test_ring();
    
    return 0;
}
