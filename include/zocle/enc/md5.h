#ifndef ZOCLE_ENC_MD5_H
#define ZOCLE_ENC_MD5_H

#include <stdio.h>
#include <stdint.h>

typedef struct {
	uint32_t state[4];	 /* state (ABCD) */
	uint32_t count[2];	 /* number of bits, modulo 2^64 (lsb first) */
	uint8_t  buffer[64]; /* input buffer */
}zcMD5Ctx;

//void make_digest(char *md5str, unsigned char *digest);
void zc_md5_init(zcMD5Ctx *);
void zc_md5_update(zcMD5Ctx *, const uint8_t *, uint32_t);
void zc_md5_final(zcMD5Ctx*, uint8_t[16]);

int zc_md5(char *md5str, const char *str, int slen);
int zc_md5_file(char *md5str, const char* filename);

#endif
