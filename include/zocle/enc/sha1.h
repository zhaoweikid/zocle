#ifndef ZOCLE_ENC_SHA1_H
#define ZOCLE_ENC_SHA1_H

#include <stdio.h>

#ifdef ZOCLE_WITH_SSL
int zc_openssl_sha1(char *sha1, const char *data, int datalen);
int zc_openssl_sha1_bcd(char *sha1, const char *data, int datalen);
int zc_openssl_sha1_base64(char *sha1, const char *data, int datalen);
#endif

int zc_sha1(char *sha, const char *data, int datalen);
int zc_sha1_bcd(char *sha, const char *data, int datalen);
int zc_sha1_base64(char *sha, const char *data, int datalen);

struct zc_sha_t;
void zc_sha1_init(struct zc_sha_t *sha_info);
void zc_sha1_update(struct zc_sha_t *sha_info, uint8_t *buffer, unsigned int count);
void zc_sha1_final(struct zc_sha_t *sha_info, unsigned char digest[20]);

#endif
