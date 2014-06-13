#include <zocle/enc/compress.h>
#include <zocle/base/defines.h>
#include <string.h>
#include <stdlib.h>
#include <zocle/mem/alloc.h>
#include <zocle/base/defines.h>
#include <zocle/log/logfile.h>
#include <zocle/str/string.h>

int 
zc_gzip_enc(char *dst, int dstlen, char *src, int srclen)
{
    z_stream c_stream;
    int err = 0;

    if (src && srclen > 0) {
        c_stream.zalloc = (alloc_func)0;
        c_stream.zfree  = (free_func)0;
        c_stream.opaque = (voidpf)0;
        
        if (deflateInit2(&c_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK)
            return ZC_ERR;
        c_stream.next_in   = (unsigned char*)src;
        c_stream.avail_in  = srclen;
        c_stream.next_out  = (unsigned char*)dst;
        c_stream.avail_out = dstlen;

        while (c_stream.avail_in != 0 && c_stream.total_out < dstlen) {
            if (deflate(&c_stream, Z_NO_FLUSH) != Z_OK) 
                return ZC_ERR;
        }
        if (c_stream.avail_in != 0) 
            return c_stream.avail_in;
        for (;;) {
            if ((err = deflate(&c_stream, Z_FINISH)) == Z_STREAM_END) 
                break;
            if (err != Z_OK) 
                return ZC_ERR;
        }
        if (deflateEnd(&c_stream) != Z_OK) 
            return ZC_ERR;
        return c_stream.total_out;
    }
    return ZC_ERR;
}

int 
zc_gzip_dec(char *dst, int dstlen, char *src, int srclen)
{
    int err = 0;
    z_stream d_stream = {0};
    static char dummy_head[2] = {
        0x8 + 0x7 * 0x10,
        (((0x8 + 0x7 * 0x10) * 0x100 + 30) / 31 * 31) & 0xFF,
    };
    d_stream.zalloc   = (alloc_func)0;
    d_stream.zfree    = (free_func)0;
    d_stream.opaque   = (voidpf)0;
    d_stream.next_in  = (unsigned char*)src;
    d_stream.avail_in = 0;
    d_stream.next_out = (unsigned char*)dst;

    //if (inflateInit2(&d_stream, -MAX_WBITS) != Z_OK) return ZC_ERR;
    if (inflateInit2(&d_stream, 47) != Z_OK) 
        return ZC_ERR;
    while (d_stream.total_out < dstlen && d_stream.total_in < srclen) {
        d_stream.avail_in = d_stream.avail_out = 1; /* force small buffers */
        if ((err = inflate(&d_stream, Z_NO_FLUSH)) == Z_STREAM_END) 
            break;
        if (err != Z_OK ){
            if(err == Z_DATA_ERROR){
                d_stream.next_in  = (uint8_t*) dummy_head;
                d_stream.avail_in = sizeof(dummy_head);
                if ((err = inflate(&d_stream, Z_NO_FLUSH)) != Z_OK) {
                    return ZC_ERR;
                }
            }else{
                return ZC_ERR;
            }
        }
    }
    if (inflateEnd(&d_stream) != Z_OK) 
        return ZC_ERR;
    return d_stream.total_out;
}

int 
zc_deflate_enc(char *dst, int dstlen, char *src, int srclen)
{
    z_stream c_stream;
    int err = 0;

    if (src && srclen > 0) {
        c_stream.zalloc = (alloc_func)0;
        c_stream.zfree  = (free_func)0;
        c_stream.opaque = (voidpf)0;
        //if (deflateInit(&c_stream, Z_DEFAULT_COMPRESSION) != Z_OK) 
        if (deflateInit2(&c_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 
            MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) != Z_OK)
            return ZC_ERR;
        c_stream.next_in    = (unsigned char*)src;
        c_stream.avail_in   = srclen;
        c_stream.next_out   = (unsigned char*)dst;
        c_stream.avail_out  = dstlen;

        while (c_stream.avail_in != 0 && c_stream.total_out < dstlen) {
            if (deflate(&c_stream, Z_NO_FLUSH) != Z_OK) 
                return ZC_ERR; 
        }   
        if (c_stream.avail_in != 0) 
            return c_stream.avail_in;
        for (;;) {
            if ((err = deflate(&c_stream, Z_FINISH)) == Z_STREAM_END) 
                break;
            if (err != Z_OK) 
                return ZC_ERR; 
        }   
        if (deflateEnd(&c_stream) != Z_OK) 
            return ZC_ERR; 
        return c_stream.total_out;
    }   
    return ZC_ERR;

}

int 
zc_deflate_dec(char *dst, int dstlen, char *src, int srclen)
{
    return 0;
}


int 
zc_zlib_enc(char *dst, int dstlen, char *src, int srclen)
{
    z_stream c_stream;
    int err = 0;

    if (src && srclen > 0) {
        c_stream.zalloc = (alloc_func)0;
        c_stream.zfree  = (free_func)0;
        c_stream.opaque = (voidpf)0;
        if (deflateInit(&c_stream, Z_DEFAULT_COMPRESSION) != Z_OK) 
            return ZC_ERR;
        c_stream.next_in    = (unsigned char*)src;
        c_stream.avail_in   = srclen;
        c_stream.next_out   = (unsigned char*)dst;
        c_stream.avail_out  = dstlen;

        while (c_stream.avail_in != 0 && c_stream.total_out < dstlen) {
            if (deflate(&c_stream, Z_NO_FLUSH) != Z_OK) 
                return ZC_ERR; 
        }   
        if (c_stream.avail_in != 0) 
            return c_stream.avail_in;
        for (;;) {
            if ((err = deflate(&c_stream, Z_FINISH)) == Z_STREAM_END) 
                break;
            if (err != Z_OK) 
                return ZC_ERR; 
        }   
        if (deflateEnd(&c_stream) != Z_OK) 
            return ZC_ERR; 
        return c_stream.total_out;
    }   
    return ZC_ERR;
}

int 
zc_zlib_dec(char *dst, int dstlen, char *src, int srclen)
{
    int err = 0;
    z_stream d_stream;

    d_stream.zalloc   = (alloc_func)0;
    d_stream.zfree    = (free_func)0;
    d_stream.opaque   = (voidpf)0;
    d_stream.next_in  = (unsigned char*)src;
    d_stream.avail_in = 0;
    d_stream.next_out = (unsigned char*)dst;

    if (inflateInit(&d_stream) != Z_OK) 
        return ZC_ERR;
    while (d_stream.total_out < dstlen && d_stream.total_in < srclen) {
        d_stream.avail_in = d_stream.avail_out = 1; /* force small buffers */
        if ((err = inflate(&d_stream, Z_NO_FLUSH)) == Z_STREAM_END) 
            break;
        if (err != Z_OK) 
            return ZC_ERR;
    }
    if (inflateEnd(&d_stream) != Z_OK) 
        return ZC_ERR;
    return d_stream.total_out;
}

zcCompress* 
zc_compress_new(uint8_t algri, int wbits)
{
    zcCompress *c = (zcCompress*)zc_malloc(sizeof(zcCompress));

    int ret = zc_compress_init(c, algri, wbits);
    if (ret != ZC_OK) {
        zc_free(c);
        return NULL;
    }
    return c;
}

int
zc_compress_init(zcCompress *c, uint8_t algri, int wbits)
{
    memset(c, 0, sizeof(zcCompress));
    c->algri = algri;
    c->wbits = wbits;
    c->isfinish = false;
    //c->isinit = true;
    c->stream.zalloc = (alloc_func)0;
    c->stream.zfree  = (free_func)0;
    c->stream.opaque = (voidpf)0;
    //c->flush  = Z_NO_FLUSH;

    if (wbits == 0) {
        c->wbits = -MAX_WBITS;
    }

    int ret;
    switch (c->algri) {
    case ZC_GZIP_DEC:
        //ret = inflateInit2(&c->stream, -MAX_WBITS);
        ret = inflateInit2(&c->stream, c->wbits);
        break;
    case ZC_GZIP_ENC:
        ret = deflateInit2(&c->stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
        break;
    default:
        return ZC_ERR;
    }
    if (ret != Z_OK) {
        ZCWARN("deflatInit2 error: %d", ret);
        return ZC_ERR;
    }
    return ZC_OK;
}

void		
zc_compress_delete(void *x)
{
    zc_compress_destroy(x);
    zc_free(x);
}

void 
zc_compress_destroy(void *x)
{
    zcCompress *c = (zcCompress*)x;
    if (!c->isfinish) {
        zc_compress_finish(c);
    }
}

int	
zc_compress_reset(zcCompress *c)
{
    int ret;
    switch (c->algri) {
    case ZC_GZIP_DEC:
        ret = inflateReset(&c->stream);
        break;
    case ZC_GZIP_ENC:
        ret = deflateReset(&c->stream);
        break;
    default:
        return ZC_ERR;
    }
    return ret;  
}

int	
zc_compress_finish(zcCompress *c)
{
    int ret = Z_OK;
    /*ret = deflate(&c->stream, Z_FINISH);
    if (ret != Z_OK)
        return ret;*/
    switch(c->algri){
    case ZC_GZIP_ENC:
        ret = deflateEnd(&c->stream);
        break;
    case ZC_GZIP_DEC:
        ret = inflateEnd(&c->stream);
        break;
    }
    if (ret != Z_OK) {
        ZCWARN("inflateEnd error: %d", ret);
        return ZC_ERR;
    }
    c->isfinish = true;
    return ZC_OK;
}

int	
zc_compress_do(zcCompress *c, uint8_t *in, int64_t inlen, uint8_t *out, int64_t outlen, bool isfinish)
{
    if (in && inlen > 0) {
        c->stream.next_in  = in;
        c->stream.avail_in = inlen;
    }
    /*if (inlen > 0) {
        c->stream.avail_in = inlen;
    }*/
    c->stream.next_out  = out;
    c->stream.avail_out = outlen;

    int err = 0;
    //uint8_t flag = Z_SYNC_FLUSH;
    uint8_t flag = Z_NO_FLUSH;
    if (isfinish) {
        flag = Z_FINISH;
    }
    int64_t lastout = c->stream.total_out;
    c->lastsize = 0;

    /*ZCINFO("1 inlen:%lld, outlen:%lld, avail_in:%d, total_in:%ld, avail_out:%d, total_out:%ld", 
            inlen, outlen, c->stream.avail_in, c->stream.total_in, 
            c->stream.avail_out, c->stream.total_out);*/

    switch (c->algri) {
        case ZC_GZIP_DEC:
            err = inflate(&c->stream, flag);
            break;
        case ZC_GZIP_ENC:
            err = deflate(&c->stream, flag);
            break;
        default:
            return ZC_ERR;
    }
    /*ZCINFO("2 avail_in:%d, total_in:%ld, avail_out:%d, total_out:%ld", 
            c->stream.avail_in, c->stream.total_in, 
            c->stream.avail_out, c->stream.total_out);
    ZCINFO("ret err: %d", err);*/
    if (err == Z_STREAM_END) {
        //ZCINFO("end stream");
        err = zc_compress_finish(c);
        if (err != Z_OK) {
            ZCWARN("compress finish error: %d", err);
            return ZC_ERR;
        }
    }else{
        if (err != Z_OK) {
            ZCWARN("compress error: %d, %ld", err, c->stream.total_out);
            if (err == Z_BUF_ERROR) {
                c->lastsize = c->stream.total_out - lastout;
                return ZC_ERR_BUF;
            }
            return ZC_ERR;
        }
        if (c->stream.avail_out == 0) {
            //ZCWARN("maybe output buffer want more size");
            c->lastsize = c->stream.total_out - lastout;
            return ZC_ERR_BUF;
        }
    }
    c->lastsize = c->stream.total_out - lastout;
    return ZC_OK;
}

int	
zc_compress_dec(zcCompress *c, uint8_t *in, int64_t inlen, zcString *out, bool isfinish)
{
    if (c->algri!=ZC_GZIP_DEC && c->algri!=ZC_DEFLATE_DEC && c->algri!=ZC_COMPRESS_DEC) {
        return ZC_ERR;
    }
    if (in && inlen <= 0) {
        ZCWARN("compress inlen==0");
        return ZC_ERR;
    }
   
    int ret = 0;
    int thelen = inlen;
    //int dolen = 0; 
    zc_str_ensure_idle_size(out, thelen*2);
    //ZCINFO("out len:%d, size:%d", out->len, out->size);
    
    zc_check(out->data);
    //c->flush = Z_NO_FLUSH;
    while (1) {
        //ZCINFO("try uncompress: %lld, out idle:%d, len:%d, size:%d", inlen, zc_str_idle(out), out->len, out->size);
        ret = zc_compress_do(c, in, inlen, (uint8_t*)&out->data[out->len], zc_str_idle(out), isfinish);
        while (ret == ZC_ERR_BUF) {
            //ZCINFO("lastsize: %d", c->lastsize);
            out->len += c->lastsize;
            //ZCINFO("maybe resize: %d, now: %d/%d", zc_str_idle(out)*2, out->len, out->size);
            zc_str_ensure_idle_size(out, out->size+8192);
            //ZCINFO("=== after resize: %d/%d ===", out->len, out->size);
            ret = zc_compress_do(c, NULL, 0, (uint8_t*)&out->data[out->len], zc_str_idle(out), isfinish);
        }
        if (ret < 0) {
            return ZC_ERR;
        }
        zc_check(out->data);
        out->len += c->lastsize;
        //ZCINFO("lastsize:%d, len:%d, size:%d", c->lastsize, out->len, out->size);
        break;
    }
    return ret;
}


