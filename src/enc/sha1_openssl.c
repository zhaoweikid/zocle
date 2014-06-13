#ifdef ZOCLE_WITH_SSL
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <zocle/log/logfile.h>
#include <zocle/base/defines.h>
#include <zocle/enc/bcd.h>
#include <zocle/enc/base64.h>


int zc_openssl_sha1(char *out, const char *data, int datalen)
{
    SHA_CTX s;

    SHA1_Init(&s);
    SHA1_Update(&s, data, datalen);
    SHA1_Final((unsigned char*)out, &s);
   
    return ZC_OK; 
}

int zc_openssl_sha1_bcd(char *out, const char *data, int datalen)
{
    char buf[20];
    zc_openssl_sha1(buf, data, datalen);
    return zc_bin2bcd(out, buf, 20);
}

int zc_openssl_sha1_base64(char *out, const char *data, int datalen)
{
    char buf[20];
    zc_openssl_sha1(buf, data, datalen);
    return zc_base64_enc(out, buf, 20);
}


#endif
