#ifndef ZOCLE_ENC_BCD_H
#define ZOCLE_ENC_BCD_H

#include <stdio.h>

int zc_bcd2bin(char *binary, char *bcd);
int zc_bin2bcd(char *bcd, char *binary, int binlen);

#endif
