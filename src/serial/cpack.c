#include <zocle/serial/cpack.h>
#include <zocle/log/logfile.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <zocle/str/string.h>

/**
 * 通过指定格式串对数据序列化。
 * 格式串中每一项为 类型
 * 类型有 i:4字节, l:8字节, h:2字节, c:1字节, f:4字节浮点, d:8字节浮点, s:\0结尾的字符串, a数组, m字典, n空对象
 *        a后的字符表示数组中每项类型,m后的字符表示value类型。
 *        比如: ai, 表示数组中的每项都是i; mc表示字典的value是字符
 * 如果类型为大写，则为数组，且通过传递参数指定个数。比如: 格式"I", 需要两个参数: array_len, array
 * 特殊字符$在第一个字符出现表示在头部写入整个数据长度（不包括此头部）。$后跟数字表示用几个字节。
 * 例子:
 *   int a[3] = {1, 2, 3};
 *   char b[3] = {'a', 'b', 'c'};
 *   pack(buf, 128, "i3sc", a, "haha", 'b');
 *   pack(buf, 128, "Isc", 3, a, "haha", 'b');
 *   pack(buf, 128, "$4Isc", 3, a, "haha", 'b');
 *   pack(buf, 128, "sc3", "haha", b);
 *   pack(buf, 128, "sC", "haha", 3, b);
 *   
 *   char c[16];
 *   int dlen;
 *   char d[3];
 *   unpack(buf, 128, "sC", c, dlen, d);
 *
 *  +------------+--------+---------+--------+---------+------+
 *  | version(1B)|flag(1B)|len(1-4B)| format |len(1-4B)| data |
 *  +------------+--------+---------+--------+---------+------+
 *
 *  flag: 
 *  +--------------+-----------+-------------+
 *  | compress(3b) | format(1b)| not used(4b)|
 *  +--------------+-----------+-------------+
 *    compress:  (0-7)
 *               0 not compress 
 *               1 zip
 *               2 lzo
 *
 *    format:    0 no format
 *               1 have format
 *
 *  len:
 *  1B: 0b?0000000   0-127
 *  2B: 0b1?00000000000000 128-16383
 *  4B: 0b11000000000000000000000000000000 16384-1073741823
 *
 * @param buf  数据缓冲区，保存序列化后的数据用，需要预先分配好
 * @param buf  前面的数据缓冲区大小
 * @param format 格式串
 * @return 正常返回整个数据包长度, 出错返回-1
 */




#define ZC_PACK_VAR(TP,TP2) \
    if (num > 0) {\
        zc_str_append_len(buf, (void*)&num, numsize);\
        TP *vals = va_arg(arg, TP *);\
        zc_str_append_len(buf, (void*)vals, sizeof(TP)*num);\
    }else{\
        TP val = va_arg(arg, TP2);\
        zc_str_append_len(buf, (void*)&val, sizeof(TP));\
    }\
    break;


#define ZC_PACK_ARRAY(TP) \
    num = va_arg(arg, int);\
    zc_str_append_len(buf, (void*)&num, numsize);\
    TP *vals = va_arg(arg, TP *);\
    zc_str_append_len(buf, (void*)vals, sizeof(TP)*num);\
    break;

#define ZC_PACK_PARSE_ONE() \
    switch(c1) {\
    case 'i': {\
        ZC_PACK_VAR(int, int)\
    }\
    case 'I': {\
        ZC_PACK_ARRAY(int)\
    }\
    case 'f': {\
        ZC_PACK_VAR(float, double)\
    }\
    case 'F': {\
        ZC_PACK_ARRAY(float)\
    }\
    case 'l': {\
        ZC_PACK_VAR(int64_t, int64_t)\
    }\
    case 'L': {\
        ZC_PACK_ARRAY(int64_t)\
    }\
    case 'd': {\
        ZC_PACK_VAR(double, double)\
    }\
    case 'D': {\
        ZC_PACK_ARRAY(double)\
    }\
    case 'c': {\
        ZC_PACK_VAR(char, int)\
    }\
    case 'C': {\
        ZC_PACK_ARRAY(char)\
    }\
    case 'h': {\
        ZC_PACK_VAR(short, int)\
    }\
    case 'H': {\
        ZC_PACK_ARRAY(short)\
    }\
    case 's': {\
        char *val = va_arg(arg, char *);\
        zc_str_append(buf, val);\
        break;\
    }\
    case 'S': {\
        zcString *val = va_arg(arg, zcString*);\
        zc_str_append(buf, val->data);\
        break; \
    }\
    default:\
        ZCERROR("format error:%c", c1);\
        return ZC_ERR;\
        break;\
    }
 

static int 
_cpack_parse(char *format, int i, int *num, int *headsize)
{
    *num = 0;
    int nexti = i + 1;
    while (format[nexti] >= '0' && format[nexti] <= '9') {
        nexti++;
    }
    if (nexti > i+1) {
        *num = atoi(&format[i+1]);
    }
    if (format[nexti] == ':') {
        int hsize = format[nexti+1] - 48;
        if (hsize <= 0 || hsize > 4) {  // size error
            ZCERROR("headsize error:%d\n", hsize);
            return ZC_ERR;
        }
        *headsize = hsize;
        nexti++; // skip :
        // skip numbers
        while (format[nexti] >= '0' && format[nexti] <= '9') {
            nexti++;
        }
    }else{
        *headsize = 1;
    }
    return nexti; 
}

static int 
_cunpack_parse(char *format, int i, int *num, int *headsize)
{
    int nexti = i + 1;
    while (format[nexti] >= '0' && format[nexti] <= '9') {
        nexti++;
    }
    *num = 0;
    if (nexti > i+1) {
        *num = atoi(&format[i+1]);
    }
    if (format[nexti] == ':') {
        int numsize = format[nexti+1] - 48;
        if (numsize == 0 || numsize > 4) {  // size error
            ZCERROR("numsize error:%d\n", numsize);
            return -1;
        }
        *headsize = numsize;
        nexti++; // skip :
        // skip numbers
        while (format[nexti] >= '0' && format[nexti] <= '9') {
            nexti++;
        }
    }
    return nexti;
}



int zc_cpack(zcString *buf, char *format, ...)
{
    //int uselen = 0; 
    va_list arg;
    int i = 0;
    int numsize = 1; // array/string len field size, max is 4
    int headlen = 0;
    int formatlen = strlen(format);

    if (format[0] == '$') { // 有头部长度部分
        headlen = format[1] - 48;
        if (headlen != 1 && headlen != 2 && headlen != 4) 
            return ZC_ERR;
        i += 2;
        zc_str_skip_len(buf, headlen);
        //uselen += headlen;
    }

    va_start(arg, format);
    char c1; //, c2, c3;
    int num, nexti;
    while ((c1 = format[i]) != 0) {
        nexti = _cpack_parse(format, i, &num, &numsize);
        if (nexti <= 0)
            return ZC_ERR;

        //ZCINFO("num:%d, numsize:%d, uselen:%d\n", num, numsize, uselen);
        switch(c1) {
        case 'i': { // int, 32bit
            ZC_PACK_VAR(int, int)
        }
        case 'I': { // int, 32b, array, and array length pass by a param
            ZC_PACK_ARRAY(int)
        }
        case 'f': { // float, 32bit
            ZC_PACK_VAR(float, double)
        }
        case 'F': { // int, 32b, array, and array length pass by a param
            ZC_PACK_ARRAY(float)
        }
        case 'l': { // long, 64bit
            ZC_PACK_VAR(int64_t, int64_t)
        }
        case 'L': {
            ZC_PACK_ARRAY(int64_t)
        }
        case 'd': { // double, 64bit
            ZC_PACK_VAR(double, double)
        }
        case 'D': {
            ZC_PACK_ARRAY(double)
        }
        case 'c': { // char, 8bit
            ZC_PACK_VAR(char, int)
        }
        case 'C': {
            ZC_PACK_ARRAY(char)
        }
        case 'h': { // short, 16bit
            ZC_PACK_VAR(short, int)
        }
        case 'H': {
            ZC_PACK_ARRAY(short)
        }
        case 's': { // string
            char *val = va_arg(arg, char *);
            zc_str_append(buf, val);
            //int vlen = strlen(val) + 1;
            //memcpy(buf + uselen, val, vlen);
            //uselen += vlen;
            break;
        }
        case 'S': { // zcString
            zcString *val = va_arg(arg, zcString*);
            zc_str_append(buf, val->data);
            break; 
        }
        default:
            ZCERROR("format error:%c", c1);
            return ZC_ERR;
            break;
        }
        //if (buflen > 0 && uselen > buflen)
        //    return ZC_ERR;
        //numsize = 1; // set default
        i = nexti;
    }
    va_end(arg);

    if (headlen > 0) {
        unsigned int blen = buf->len - headlen; //uselen - headlen;  
        memcpy(buf->data, &blen, headlen);
    }
    return buf->len; 
}

int zc_cpack_append(zcString *buf, char *format, ...)
{
    //int uselen = 0; 
    int oldlen = buf->len;
    va_list arg;
    int i = 0;
    int numsize = 1; // array/string len field size, max is 4

    va_start(arg, format);
    char c1; //, c2, c3;
    int num, nexti;
    while ((c1 = format[i]) != 0) {
        nexti = _cpack_parse(format, i, &num, &numsize);
        if (nexti <= 0)
            return ZC_ERR;

        //ZCINFO("num:%d, numsize:%d, uselen:%d\n", num, numsize, uselen);
        switch(c1) {
        case 'i': { // int, 32bit
            ZC_PACK_VAR(int, int)
        }
        case 'I': { // int, 32b, array, and array length pass by a param
            ZC_PACK_ARRAY(int)
        }
        case 'f': { // float, 32bit
            ZC_PACK_VAR(float, double)
        }
        case 'F': { // int, 32b, array, and array length pass by a param
            ZC_PACK_ARRAY(float)
        }
        case 'l': { // long, 64bit
            ZC_PACK_VAR(int64_t, int64_t)
        }
        case 'L': {
            ZC_PACK_ARRAY(int64_t)
        }
        case 'd': { // double, 64bit
            ZC_PACK_VAR(double, double)
        }
        case 'D': {
            ZC_PACK_ARRAY(double)
        }
        case 'c': { // char, 8bit
            ZC_PACK_VAR(char, int)
        }
        case 'C': {
            ZC_PACK_ARRAY(char)
        }
        case 'h': { // short, 16bit
            ZC_PACK_VAR(short, int)
        }
        case 'H': {
            ZC_PACK_ARRAY(short)
        }
        case 's': { // string
            char *val = va_arg(arg, char *);
            zc_str_append(buf, val);
            break;
        }
        case 'S': {
            zcString *val = va_arg(arg, zcString*);
            zc_str_append(buf, val->data);
            break; 
        }
        default:
            ZCERROR("format error:%c", c1);
            return ZC_ERR;
            break;
        }
        //if (buflen > 0 && uselen > buflen)
        //    return ZC_ERR;
        //numsize = 1; // set default
        i = nexti;
    }
    va_end(arg);

    return buf->len-oldlen; 
}

int zc_cpack_end(zcString *buf, int headlen)
{
    if (buf->len >= 4) {
        unsigned int blen = buf->len - headlen; //uselen - headlen;  
        memcpy(buf->data, &blen, headlen);
    }
    return ZC_OK;
}

#define ZC_UNPACK_VAR(TP) \
    if (num > 0) {\
        if (numsize == 1) {\
            uint8_t *sz = va_arg(arg, uint8_t *);\
            memcpy(sz, buf+uselen, numsize);\
            uselen += sizeof(uint8_t);\
            num = *sz;\
        }else{\
            uint32_t *sz = va_arg(arg, uint32_t *);\
            memcpy(sz, buf+uselen, numsize);\
            uselen += sizeof(uint32_t);\
            num = *sz;\
        }\
        TP *vals = va_arg(arg, TP *);\
        memcpy(vals, buf+uselen, sizeof(TP)*num);\
        uselen += sizeof(TP) * num;\
    }else{\
        TP *val = va_arg(arg, TP*);\
        memcpy(val, buf+uselen, sizeof(TP));\
        uselen += sizeof(TP);\
    }\
    break;



/**
 * 反序列化
 * @param buf 序列化数据
 * @param buf 序列化数据长度
 * @param format 格式串
 * @return 正常返回反序列化的数据长度，错误返回-1
 */
int zc_cunpack(char *buf, int buflen, char *format, ...)
{
    int uselen = 0; 
    va_list arg;
    int i = 0, n;
    int numsize = 1; // array/string len field size, max is 4
    char ch;
    int nexti, num; 
    va_start(arg, format);
    while (format[i]) {
        nexti = _cunpack_parse(format, i, &num, &numsize);
        if (nexti <= 0)
            return ZC_ERR;

        ch = format[i];
        if (ch <= 'Z') {
            ch += 32;
            num = 10; // 数字是多少不重要，只要大于0
        }
        //ZCINFO("num:%d, numsize:%d, uselen:%d\n", num, numsize, uselen);
        switch(ch) {
        case 'f': {
            ZC_UNPACK_VAR(float)
        }
        case 'i': {
            ZC_UNPACK_VAR(int)
        }
        case 'd':
        case 'l': {
            ZC_UNPACK_VAR(int64_t)
        }
        case 'c': {
            ZC_UNPACK_VAR(char)
        }
        case 'h': {
            ZC_UNPACK_VAR(int16_t)
        }
        case 's': {
            char *val = va_arg(arg, char *); 
            n = 0;
            char *data = buf + uselen;
            while (data[n] != 0) {
                val[n] = data[n];
                n++;
            }
            val[n] = 0;
            uselen += n + 1;
            break;
        }
        case 'S': {
            zcString *val = va_arg(arg, zcString *); 
            n = 0;
            char *data = buf + uselen;
            while (data[n] != 0) {
                //val[n] = data[n];
                n++;
            }
            //val[n] = 0;
            zc_str_append_len(val, data, n);
            uselen += n + 1;
            break;
        }
        default:
            return -1;
        }
        if (buflen > 0 && uselen > buflen)
            return -1;

        numsize = 1;
        i = nexti;
    }
    va_end(arg);

    return uselen;
}



