#include <zocle/enc/bcd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char hexdigits[] = "0123456789ABCDEF";

static unsigned char str_to_char (char a, char b)
{
    char encoder[3] = {'\0','\0','\0'};
    encoder[0] = a;
    encoder[1] = b;
    return (char) strtol(encoder,NULL,16);
}

int zc_bcd2bin(char *binary, char *bcd)
{
    char *index = binary;
    while ((*bcd) && (*(bcd +1))) {
        char a=(*bcd);
        char b=(*(bcd +1));
        *index = str_to_char(a, b);
        index++;
        bcd+=2;
    }
    *index = '\0';
    
    return index-binary;
}

int zc_bin2bcd(char *bcd, char *binary, int binlen)
{
    char *bytes = binary;
    char *hex = bcd;
   
    int i;
    for (i=0; i<binlen; ++i){
        const unsigned char c = *bytes++;
        *hex++ = hexdigits[(c >> 4) & 0xF];
        *hex++ = hexdigits[(c ) & 0xF];
    }
    *hex = 0;
    return hex-bcd;
}

#ifdef BCDTEST
int main()
{
    char *binary = "我们的测试"; 
    char bcd[100]; // length of bcd must = (length of binary *2)+1
    int ret; 
   
    printf("binary len:%d\n", strlen(binary)); 
    ret = binary2bcd(binary, strlen(binary), bcd); 
    printf("binary2bcd ret:%d\n", ret);
    bcd[ret] = 0;
    printf("bcd:%s\n", bcd);

    char bin[128] = {0};
    ret = bcd2binary(bcd, bin);
    printf("bcd2binary ret:%d\n", ret);
    printf("bin:%s\n", bin);
     

    return 0;
}
#endif

