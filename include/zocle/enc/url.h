#ifndef ZOCLE_ENC_URL_H
#define ZOCLE_ENC_URL_H

#include <stdio.h>

int zc_url_enc(char *dst, char *src, int srclen);
int zc_url_dec(char *dst, char *src, int srclen);

int zc_url_part_enc(char *dst, char *src, int srclen);
int zc_url_part_dec(char *dst, char *src, int srclen);

int zc_url_unsafe_enc(char *dst, char *src, int srclen);
int zc_url_unsafe_dec(char *dst, char *src, int srclen);

#endif
