/**
 * @file convert.c
 * @author zhaowei
 * @brief 对字符串做编码转换，可能对unicode转换有问题
 */
#ifdef ZOCLE_WITH_ICONV

#include <zocle/str/convert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <iconv.h>
#include <zocle/base/defines.h>

int
zc_iconv_convert(const char *from, const char *to, const char *src, int srclen, char* save, int savelen)
{
    iconv_t cd; 
    char *inbuf  = (char*)src;
    char *outbuf = save;
    size_t outbufsize = savelen;
    int status = 0;
    size_t  savesize = 0;
    size_t inbufsize = srclen;
    const char* inptr  = inbuf;
    size_t      insize = inbufsize;
    char* outptr   = outbuf;
    size_t outsize = outbufsize;

    if (inbufsize == 0) {
        status = ZC_ERR_EMPTY; 
        goto done;
    }   
    cd = iconv_open(to, from);
    if (cd == (iconv_t)-1) 
        return ZC_ERR; 
    iconv(cd, NULL, NULL, NULL, NULL);
    while (insize > 0) {
        size_t res = iconv(cd,(char**)&inptr, &insize, &outptr, &outsize);
        if (outptr != outbuf) {
            int saved_errno = errno;
            int outsize = outptr - outbuf;
            memcpy(save+savesize, outbuf, outsize);
            savesize += outsize;
            errno = saved_errno;
        }   
        if (res == (size_t)(-1)) {
            if (errno == EILSEQ) {
                int one = 1;
                iconvctl(cd, ICONV_SET_DISCARD_ILSEQ, &one);
                status = -3; 
            } else if (errno == EINVAL) {
                if (inbufsize == 0) {
                    status = -4; 
                    goto done;
                } else {
                    break;
                }
            } else if (errno == E2BIG) {
                status = ZC_ERR_BUF;
                goto done;
            } else {
                status = ZC_ERR;
                goto done;
            }
        }
    }
    //status = strlen(save);
    status = savesize;
done:
    iconv_close(cd);
    return status;
}


/*
static int 
zc_convert (iconv_t cd, char *inbuf, int inbufsize, 
            char *outbuf, int outbufsize, int discard_unconvertible)
{
    int status = 0;
    int len = 0;
    char  *inptr = inbuf;
    size_t insize = inbufsize;

    iconv(cd,NULL,NULL,NULL,NULL);
    while (insize > 0) { // 需要转的字节数
        char* outptr = outbuf + len;
        size_t outsize = outbufsize - len;
        size_t oldoutsize = outsize;
        //printf("1 insize: %d, outsize: %d, len: %d\n", insize, outsize, len);
        size_t res = iconv(cd,&inptr,&insize,&outptr,&outsize);
        //printf("insize: %d, res: %d, outsize: %d, outbufsize: %d\n", insize, res, outsize, outbufsize);
        if (outptr != outbuf) {
            int saved_errno = errno;
            errno = saved_errno;
        }
        if (res == (size_t)(-1)) {
            if (errno == EILSEQ) {
                if (discard_unconvertible == 1) { // 如果忽略错误
                    int one = 1;
                    iconvctl(cd,ICONV_SET_DISCARD_ILSEQ,&one);
                    discard_unconvertible = 2;
                    status = 1;

                    len += oldoutsize - outsize;
                } else {
                    status = 1;
                    goto done;
                }
            } else if (errno == EINVAL) {
                if (inbufsize == 0 || insize > 4096) {
                    status = 1;
                    goto done;
                } else {
                    if (insize > 0) {
                        // Like memcpy(inbuf+4096-insize,inptr,insize), except that
                        //   we cannot use memcpy here, because source and destination
                        //   regions may overlap.
                        char* restptr = inbuf+4096-insize;
                        do { *restptr++ = *inptr++; } while (--insize > 0);
                    }
                    break;
                }
            } else if (errno == E2BIG) { // 转换后的存储空间不够
                if (outptr==outbuf) {
                    status = -2;
                    goto done;
                }
            } else { // 不知名错误?
                status = 1;
                goto done;
            }
        }else{
            len += oldoutsize - outsize;
        }
    }
done:
    outbuf[len] = 0;
    return status;
}*/


/*int 
zc_iconv_convert(char *fromcode, char *tocode, char *tbuf, int tbuflen, 
                 char *fbuf, int fbuflen)
{
    #iconv_t cd;
    #int     ret = 0; 

    #cd = iconv_open(tocode,fromcode);
    #if (cd == (iconv_t)-1) {
    #    return -1;
    #}
    #ret = zc_convert(cd, fbuf, fbuflen, tbuf, tbuflen, 1);
    #iconv_close(cd);
    #
    #if (ret < 0) {
    #    return ret;
    #}else{
    #    return strlen(tbuf);
    #}

    return zc_convert2(fromcode, tocode, fbuf, fbuflen, tbuf, tbuflen);
}
*/

/*
int main (int argc, char* argv[])
{
    char  fbuf[1024], tbuf[1024] = {0};
    int   fbuflen, tbuflen = 1023, ret;
    char  *tocode = "UTF-8";
    char  *fromcode = "GBK";

    
    strcpy(fbuf, "我的测试一个");
    fbuflen = strlen(fbuf);
    
    ret = iconv_convert(fromcode, tocode, tbuf, tbuflen, fbuf, fbuflen);
    
    //printf("len: %d\n", ret);
    //printf("outptr: %s\n", tbuf);


    return 0;
}*/



int 
zc_utf8_to_ucs2(const char *f, int flen, char *t, int tlen)
{
    int out = 0; 
    uint16_t v = 0;
    char *vc = (char *)&v;
    uint8_t ch;

    int i = 0; 
    while(i < flen) {
        ch = f[i];
        if (ch >> 7 == 0) { 
            v = f[i];
            i++;
        }else if (ch >> 5 == 6){
            v = f[i]&0x1f;
            v <<= 6;
            v = v | (f[i+1]&0x3f);
            i+= 2;
        }else if (ch >> 4 == 14){ 
            v = f[i]&0x0f;
            v <<= 10;
            v = v | ((f[i+1]&0x3f)<<6);
            v = v | (f[i+2]&0x3f);
            i += 3;
        }else{
            return -1;
        }
        //t[out] = v;
        t[out] = vc[0]; 
        out++;
        t[out] = vc[1];
        out++;
    }    
    return out;  
}


int 
zc_ucs2_to_utf8(const char *f, int flen, char *t, int tlen)
{
    uint16_t v;
    char *vc = (char *)&v;
    int i;
    int out = 0;

    while (i<flen) {
        vc[0] = f[i];
        i++;
        vc[1] = f[i];
        i++;    

        if (v < 0x0080) { // 1B
            t[out] = vc[0];
            out++;
        }else if (v < 0x0800) {
            t[out] = 0xe0 | ((0x07&vc[0])<<2) | ((0xc0&vc[1])>>6);
            out++;
            t[out] = 0x80 | (0x3f&vc[1]);
            out++;
        }else{
        }
    } 
    
    return out;
}


#endif
