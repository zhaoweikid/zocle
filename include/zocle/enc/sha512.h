#ifndef ZOCLE_ENC_SHA512_H
#define ZOCLE_ENC_SHA512_H

#include <stdio.h>

int zc_sha512(char *sha512, const char *data, int datalen);
int zc_sha512_bcd(char *sha512, const char *data, int datalen);
int zc_sha512_base64(char *sha512, const char *data, int datalen);

int zc_sha384(char *sha384, const char *data, int datalen);
int zc_sha384_bcd(char *sha384, const char *data, int datalen);
int zc_sha384_base64(char *sha384, const char *data, int datalen);


#endif
