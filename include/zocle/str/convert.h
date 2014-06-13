#ifdef ZOCLE_WITH_ICONV

#ifndef ZOCLE_STRING_CONVERT_H
#define ZOCLE_STRING_CONVERT_H

#include <stdio.h>

int zc_iconv_convert(const char *fchar, const char *tchar, 
                    const char *f, int flen, 
				    char *t, int tlen);

int zc_utf8_to_ucs2(const char *f, int flen, char *t, int tlen);
int zc_ucs2_to_utf8(const char *f, int flen, char *t, int tlen);

#endif

#endif
