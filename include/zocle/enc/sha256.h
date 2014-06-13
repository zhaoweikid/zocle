#ifndef ZOCLE_ENC_SHA256_H
#define ZOCLE_ENC_SHA256_H

#include <stdio.h>

int zc_sha256(char *sha256, const char *data, int datalen);
int zc_sha256_bcd(char *sha256, const char *data, int datalen);
int zc_sha256_base64(char *sha256, const char *data, int datalen);

int zc_sha224(char *sha224, const char *data, int datalen);
int zc_sha224_bcd(char *sha224, const char *data, int datalen);
int zc_sha224_base64(char *sha224, const char *data, int datalen);

#endif
