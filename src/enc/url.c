#include <zocle/enc/url.h>
#include <zocle/base/defines.h>
#include <string.h>
#include <ctype.h>

static int 
hex_pair_value(const char * code) {
    int value = 0;
    const char * pch = code;
    for (;;) {
        int digit = *pch++;
        if (digit >= '0' && digit <= '9') {
            value += digit - '0';
        }
        else if (digit >= 'A' && digit <= 'F') {
            value += digit - 'A' + 10;
        }
        else if (digit >= 'a' && digit <= 'f') {
            value += digit - 'a' + 10;
        }
        else {
            return -1;
        }
        if (pch == code + 2)
            return value;
        value <<= 4;
    }
}

static int 
internal_url_decode(const char *source, int srclen, char *dest, bool encode_space_as_plus) {
    char *start = dest;
    const char *srcend = source + srclen;
    while (*source && source < srcend) {
        switch (*source) {
            case '+':
                if (encode_space_as_plus) {
                    *(dest++) = ' ';
                } else {
                    *dest++ = *source;
                }
                break;
            case '%':
                if (source[1] && source[2]) {
                    int value = hex_pair_value(source + 1);
                    if (value >= 0) {
                        *(dest++) = value;
                        source += 2;
                    }
                    else {
                        *dest++ = '?';
                    }
                }
                else {
                    *dest++ = '?';
                }
                break;
            default:
                *dest++ = *source;
        }
        source++;
    }
    *dest = 0;
    return dest - start;
}

static bool 
is_valid_url_char(char ch, bool unsafe_only) {
    if (unsafe_only) {
        return !(ch <= ' ' || strchr("\\\"^&`<>[]{}", ch));
    } else {
        return isalnum(ch) || strchr("-_.!~*'()", ch);
    }
}

static int 
internal_url_encode(const char *source, int srclen, char *dest,
    bool encode_space_as_plus, bool unsafe_only) {
    static const char *digits = "0123456789ABCDEF";
    /*if (max == 0) {
        return 0;
    }*/
    if (srclen <= 0) srclen = strlen(source);

    char *start = dest;
    const char *srcend = source + srclen;
    //while ((unsigned)(dest - start) < max && *source && source < srcend) {
    while (*source && source < srcend) {
        unsigned char ch = (unsigned char)(*source);
        if (*source == ' ' && encode_space_as_plus && !unsafe_only) {
            *dest++ = '+';
        } else if (is_valid_url_char(ch, unsafe_only)) {
            *dest++ = *source;
        } else {
            /*if ((unsigned)(dest - start) + 4 > max) {
                break;
            }*/
            *dest++ = '%';
            *dest++ = digits[(ch >> 4) & 0x0F];
            *dest++ = digits[       ch & 0x0F];
        }
        source++;
    }
    //ASSERT(static_cast<unsigned int>(dest - start) < max);
    *dest = 0;
    return dest - start;
}


int 
zc_url_enc(char *dst, char *src, int srclen)
{
    return internal_url_encode(src, srclen, dst, true, false);    
}

int 
zc_url_dec(char *dst, char *src, int srclen)
{
    return internal_url_decode(src, srclen, dst, true);
}

int 
zc_url_part_enc(char *dst, char *src, int srclen)
{
    return internal_url_encode(src, srclen, dst, false, false);
}

int 
zc_url_part_dec(char *dst, char *src, int srclen)
{
    return internal_url_decode(src, srclen, dst, false);
}

int 
zc_url_unsafe_enc(char *dst, char *src, int srclen)
{
    return internal_url_encode(src, srclen, dst, false, true);
}

int 
zc_url_unsafe_dec(char *dst, char *src, int srclen)
{
    return internal_url_decode(src, srclen, dst, true);
}

