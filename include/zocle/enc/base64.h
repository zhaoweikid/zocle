#ifndef ZOCLE_BASE64_H
#define ZOCLE_BASE64_H

#include <stdio.h>

int zc_base64_enc(char *output, const char *input, int inputsz);
int zc_base64_enc_multiline(char *output, const char *input, int inputsz);
int zc_base64_dec(char* output, const char *input, int inputsz);
//int zc_base64_dec(void* output, unsigned char *input, size_t inputsz, unsigned char *discarded);

#endif
