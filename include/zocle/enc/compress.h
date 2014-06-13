#ifndef ZOCLE_ENC_COMPRESS_H
#define ZOCLE_ENC_COMPRESS_H

#include <stdio.h>
#include <stdint.h>
#include <zlib.h>
#include <zocle/base/defines.h>
#include <zocle/str/string.h>

#define ZC_GZIP_ENC		1
#define ZC_GZIP_DEC		2
#define ZC_DEFLATE_ENC	3
#define ZC_DEFLATE_DEC  4	
#define ZC_COMPRESS_ENC	5
#define ZC_COMPRESS_DEC	6

int zc_gzip_enc(char *dst, int dstlen, char *src, int srclen);
int zc_gzip_dec(char *dst, int dstlen, char *src, int srclen);

int zc_deflate_enc(char *dst, int dstlen, char *src, int srclen);
int zc_deflate_dec(char *dst, int dstlen, char *src, int srclen);

int zc_zlib_enc(char *dst, int dstlen, char *src, int srclen);
int zc_zlib_dec(char *dst, int dstlen, char *src, int srclen);

typedef struct zc_compress_t
{
	z_stream stream;
	uint8_t  algri;
	int		 wbits;
	bool	 isfinish;
	int		 lastsize;
}zcCompress;

zcCompress* zc_compress_new(uint8_t algri, int wbits);
void		zc_compress_delete(void *);
int			zc_compress_init(zcCompress *, uint8_t algri, int wbits);
void		zc_compress_destroy(void *x);

int			zc_compress_reset(zcCompress *);
void		zc_compress_destroy(void *x);
int			zc_compress_finish(zcCompress *);
int			zc_compress_do(zcCompress*, uint8_t *in, int64_t inlen, uint8_t *out, int64_t outlen, bool); 
int			zc_compress_dec(zcCompress*, uint8_t *in, int64_t inlen, zcString *out, bool); 

#endif
