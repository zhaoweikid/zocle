#ifndef ZOCLE_UTILS_FUNCS_H
#define ZOCLE_UTILS_FUNCS_H

#include <stdio.h>
#include <stdint.h>

int zc_cmp_int(void *a, void *b, int);
int zc_cmp_int_ptr(void *a, void *b, int);
int zc_cmp_pint(void *a, void *b, int);
int zc_cmp_float_ptr(void *a, void *b, int);
int zc_cmp_double_ptr(void *a, void *b, int);
int zc_cmp_str(void *a, void *b, int);
int zc_cmp_str_ptr(void *a, void *b, int);
int zc_cmp_pstr(void *a, void *b, int);
int zc_cmp_obj(void *a, void *b, int);
int zc_cmp_simple(void *a, void *b, int);

uint32_t zc_hash_bkdr(void *str, int len);
uint32_t zc_hash_rs(void *str, int len);
uint32_t zc_hash_ap(void *str, int len);
uint32_t zc_hash_js(void *str, int len);
uint32_t zc_hash_elf(void *str, int len);


int zc_format_hex(char *out, const char *data, int len);

#endif
