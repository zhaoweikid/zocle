#ifndef ZOCLE_ZC_QP_H
#define ZOCLE_ZC_QP_H

#include <stdio.h>
#include <stdint.h>
#include <ctype.h>

int zc_qp_dec(char *dst, const char* src, uint32_t len);
int zc_qp_enc(char *dst, const char* src, uint32_t len);

#endif
