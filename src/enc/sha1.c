#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <zocle/enc/sha1.h>
#include <zocle/base/defines.h>
#include <zocle/enc/bcd.h>
#include <zocle/enc/base64.h>

/* Endianness testing and definitions */
#define TestEndianness(variable) {int i=1; variable=PCT_BIG_ENDIAN;\
        if (*((char*)&i)==1) variable=PCT_LITTLE_ENDIAN;}

#define PCT_LITTLE_ENDIAN 1
#define PCT_BIG_ENDIAN 0

/* The SHA block size and message digest sizes, in bytes */

#define SHA_BLOCKSIZE    64
#define SHA_DIGESTSIZE  20

/* The structure for storing SHS info */

typedef struct zc_sha_t{
    uint32_t digest[5];                /* Message digest */
    uint32_t count_lo, count_hi;       /* 64-bit bit count */
    uint8_t data[SHA_BLOCKSIZE];       /* SHA data buffer */
    int Endianness;
    int local;                          /* unprocessed amount in data */
} zcSHA;

/* When run on a little-endian CPU we need to perform byte reversal on an
   array of longwords. */

static void longReverse(uint32_t *buffer, int byteCount, int Endianness)
{
    uint32_t value;

    if ( Endianness == PCT_BIG_ENDIAN )
        return;

    byteCount /= sizeof(*buffer);
    while (byteCount--) {
        value = *buffer;
        value = ( ( value & 0xFF00FF00L ) >> 8  ) | \
                ( ( value & 0x00FF00FFL ) << 8 );
        *buffer++ = ( value << 16 ) | ( value >> 16 );
    }
}

/*static void zc_sha_copy(zcSHA *src, zcSHA *dest)
{
    dest->Endianness = src->Endianness;
    dest->local = src->local;
    dest->count_lo = src->count_lo;
    dest->count_hi = src->count_hi;
    memcpy(dest->digest, src->digest, sizeof(src->digest));
    memcpy(dest->data, src->data, sizeof(src->data));
}*/


/* ------------------------------------------------------------------------
 *
 * This code for the SHA algorithm was noted as public domain. The original
 * headers are pasted below.
 *
 * Several changes have been made to make it more compatible with the
 * Python environment and desired interface.
 *
 */

/* NIST Secure Hash Algorithm */
/* heavily modified by Uwe Hollerbach <uh@alumni.caltech edu> */
/* from Peter C. Gutmann's implementation as found in */
/* Applied Cryptography by Bruce Schneier */
/* Further modifications to include the "UNRAVEL" stuff, below */

/* This code is in the public domain */

/* UNRAVEL should be fastest & biggest */
/* UNROLL_LOOPS should be just as big, but slightly slower */
/* both undefined should be smallest and slowest */

#define UNRAVEL
/* #define UNROLL_LOOPS */

/* The SHA f()-functions.  The f1 and f3 functions can be optimized to
   save one boolean operation each - thanks to Rich Schroeppel,
   rcs@cs.arizona.edu for discovering this */

/*#define f1(x,y,z)     ((x & y) | (~x & z))            // Rounds  0-19 */
#define f1(x,y,z)       (z ^ (x & (y ^ z)))             /* Rounds  0-19 */
#define f2(x,y,z)       (x ^ y ^ z)                     /* Rounds 20-39 */
/*#define f3(x,y,z)     ((x & y) | (x & z) | (y & z))   // Rounds 40-59 */
#define f3(x,y,z)       ((x & y) | (z & (x | y)))       /* Rounds 40-59 */
#define f4(x,y,z)       (x ^ y ^ z)                     /* Rounds 60-79 */

/* SHA constants */

#define CONST1          0x5a827999L                     /* Rounds  0-19 */
#define CONST2          0x6ed9eba1L                     /* Rounds 20-39 */
#define CONST3          0x8f1bbcdcL                     /* Rounds 40-59 */
#define CONST4          0xca62c1d6L                     /* Rounds 60-79 */

/* 32-bit rotate */

#define R32(x,n)        ((x << n) | (x >> (32 - n)))

/* the generic case, for when the overall rotation is not unraveled */

#define FG(n)   \
    T = R32(A,5) + f##n(B,C,D) + E + *WP++ + CONST##n;  \
    E = D; D = C; C = R32(B,30); B = A; A = T

/* specific cases, for when the overall rotation is unraveled */

#define FA(n)   \
    T = R32(A,5) + f##n(B,C,D) + E + *WP++ + CONST##n; B = R32(B,30)

#define FB(n)   \
    E = R32(T,5) + f##n(A,B,C) + D + *WP++ + CONST##n; A = R32(A,30)

#define FC(n)   \
    D = R32(E,5) + f##n(T,A,B) + C + *WP++ + CONST##n; T = R32(T,30)

#define FD(n)   \
    C = R32(D,5) + f##n(E,T,A) + B + *WP++ + CONST##n; E = R32(E,30)

#define FE(n)   \
    B = R32(C,5) + f##n(D,E,T) + A + *WP++ + CONST##n; D = R32(D,30)

#define FT(n)   \
    A = R32(B,5) + f##n(C,D,E) + T + *WP++ + CONST##n; C = R32(C,30)

/* do SHA transformation */

static void
zc_sha_transform(zcSHA *sha_info)
{
    int i;
    uint32_t T, A, B, C, D, E, W[80], *WP;

    memcpy(W, sha_info->data, sizeof(sha_info->data));
    longReverse(W, (int)sizeof(sha_info->data), sha_info->Endianness);

    for (i = 16; i < 80; ++i) {
        W[i] = W[i-3] ^ W[i-8] ^ W[i-14] ^ W[i-16];

        /* extra rotation fix */
        W[i] = R32(W[i], 1);
    }
    A = sha_info->digest[0];
    B = sha_info->digest[1];
    C = sha_info->digest[2];
    D = sha_info->digest[3];
    E = sha_info->digest[4];
    WP = W;
#ifdef UNRAVEL
    FA(1); FB(1); FC(1); FD(1); FE(1); FT(1); FA(1); FB(1); FC(1); FD(1);
    FE(1); FT(1); FA(1); FB(1); FC(1); FD(1); FE(1); FT(1); FA(1); FB(1);
    FC(2); FD(2); FE(2); FT(2); FA(2); FB(2); FC(2); FD(2); FE(2); FT(2);
    FA(2); FB(2); FC(2); FD(2); FE(2); FT(2); FA(2); FB(2); FC(2); FD(2);
    FE(3); FT(3); FA(3); FB(3); FC(3); FD(3); FE(3); FT(3); FA(3); FB(3);
    FC(3); FD(3); FE(3); FT(3); FA(3); FB(3); FC(3); FD(3); FE(3); FT(3);
    FA(4); FB(4); FC(4); FD(4); FE(4); FT(4); FA(4); FB(4); FC(4); FD(4);
    FE(4); FT(4); FA(4); FB(4); FC(4); FD(4); FE(4); FT(4); FA(4); FB(4);
    sha_info->digest[0] += E;
    sha_info->digest[1] += T;
    sha_info->digest[2] += A;
    sha_info->digest[3] += B;
    sha_info->digest[4] += C;
#else /* !UNRAVEL */
#ifdef UNROLL_LOOPS
    FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1);
    FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1);
    FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2);
    FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2);
    FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3);
    FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3);
    FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4);
    FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4);
#else /* !UNROLL_LOOPS */
    for (i =  0; i < 20; ++i) { FG(1); }
    for (i = 20; i < 40; ++i) { FG(2); }
    for (i = 40; i < 60; ++i) { FG(3); }
    for (i = 60; i < 80; ++i) { FG(4); }
#endif /* !UNROLL_LOOPS */
    sha_info->digest[0] += A;
    sha_info->digest[1] += B;
    sha_info->digest[2] += C;
    sha_info->digest[3] += D;
    sha_info->digest[4] += E;
#endif /* !UNRAVEL */
}

/* initialize the SHA digest */

void
zc_sha1_init(zcSHA *sha_info)
{
    TestEndianness(sha_info->Endianness)

    sha_info->digest[0] = 0x67452301L;
    sha_info->digest[1] = 0xefcdab89L;
    sha_info->digest[2] = 0x98badcfeL;
    sha_info->digest[3] = 0x10325476L;
    sha_info->digest[4] = 0xc3d2e1f0L;
    sha_info->count_lo = 0L;
    sha_info->count_hi = 0L;
    sha_info->local = 0;
}

/* update the SHA digest */

void
zc_sha1_update(zcSHA *sha_info, uint8_t *buffer, unsigned int count)
{
    unsigned int i;
    uint32_t clo;

    clo = sha_info->count_lo + ((uint32_t) count << 3);
    if (clo < sha_info->count_lo) {
        ++sha_info->count_hi;
    }
    sha_info->count_lo = clo;
    sha_info->count_hi += (uint32_t) count >> 29;
    if (sha_info->local) {
        i = SHA_BLOCKSIZE - sha_info->local;
        if (i > count) {
            i = count;
        }
        memcpy(((uint8_t *) sha_info->data) + sha_info->local, buffer, i);
        count -= i;
        buffer += i;
        sha_info->local += i;
        if (sha_info->local == SHA_BLOCKSIZE) {
            zc_sha_transform(sha_info);
        }
        else {
            return;
        }
    }
    while (count >= SHA_BLOCKSIZE) {
        memcpy(sha_info->data, buffer, SHA_BLOCKSIZE);
        buffer += SHA_BLOCKSIZE;
        count -= SHA_BLOCKSIZE;
        zc_sha_transform(sha_info);
    }
    memcpy(sha_info->data, buffer, count);
    sha_info->local = count;
}

/* finish computing the SHA digest */

void
zc_sha1_final(zcSHA *sha_info, unsigned char digest[20])
{
    int count;
    uint32_t lo_bit_count, hi_bit_count;

    lo_bit_count = sha_info->count_lo;
    hi_bit_count = sha_info->count_hi;
    count = (int) ((lo_bit_count >> 3) & 0x3f);
    ((uint8_t *) sha_info->data)[count++] = 0x80;
    if (count > SHA_BLOCKSIZE - 8) {
        memset(((uint8_t *) sha_info->data) + count, 0,
               SHA_BLOCKSIZE - count);
        zc_sha_transform(sha_info);
        memset((uint8_t *) sha_info->data, 0, SHA_BLOCKSIZE - 8);
    }
    else {
        memset(((uint8_t *) sha_info->data) + count, 0,
               SHA_BLOCKSIZE - 8 - count);
    }

    /* GJS: note that we add the hi/lo in big-endian. zc_sha_transform will
       swap these values into host-order. */
    sha_info->data[56] = (hi_bit_count >> 24) & 0xff;
    sha_info->data[57] = (hi_bit_count >> 16) & 0xff;
    sha_info->data[58] = (hi_bit_count >>  8) & 0xff;
    sha_info->data[59] = (hi_bit_count >>  0) & 0xff;
    sha_info->data[60] = (lo_bit_count >> 24) & 0xff;
    sha_info->data[61] = (lo_bit_count >> 16) & 0xff;
    sha_info->data[62] = (lo_bit_count >>  8) & 0xff;
    sha_info->data[63] = (lo_bit_count >>  0) & 0xff;
    zc_sha_transform(sha_info);
    digest[ 0] = (unsigned char) ((sha_info->digest[0] >> 24) & 0xff);
    digest[ 1] = (unsigned char) ((sha_info->digest[0] >> 16) & 0xff);
    digest[ 2] = (unsigned char) ((sha_info->digest[0] >>  8) & 0xff);
    digest[ 3] = (unsigned char) ((sha_info->digest[0]      ) & 0xff);
    digest[ 4] = (unsigned char) ((sha_info->digest[1] >> 24) & 0xff);
    digest[ 5] = (unsigned char) ((sha_info->digest[1] >> 16) & 0xff);
    digest[ 6] = (unsigned char) ((sha_info->digest[1] >>  8) & 0xff);
    digest[ 7] = (unsigned char) ((sha_info->digest[1]      ) & 0xff);
    digest[ 8] = (unsigned char) ((sha_info->digest[2] >> 24) & 0xff);
    digest[ 9] = (unsigned char) ((sha_info->digest[2] >> 16) & 0xff);
    digest[10] = (unsigned char) ((sha_info->digest[2] >>  8) & 0xff);
    digest[11] = (unsigned char) ((sha_info->digest[2]      ) & 0xff);
    digest[12] = (unsigned char) ((sha_info->digest[3] >> 24) & 0xff);
    digest[13] = (unsigned char) ((sha_info->digest[3] >> 16) & 0xff);
    digest[14] = (unsigned char) ((sha_info->digest[3] >>  8) & 0xff);
    digest[15] = (unsigned char) ((sha_info->digest[3]      ) & 0xff);
    digest[16] = (unsigned char) ((sha_info->digest[4] >> 24) & 0xff);
    digest[17] = (unsigned char) ((sha_info->digest[4] >> 16) & 0xff);
    digest[18] = (unsigned char) ((sha_info->digest[4] >>  8) & 0xff);
    digest[19] = (unsigned char) ((sha_info->digest[4]      ) & 0xff);
}

/*
 * End of copied SHA code.
 *
 * ------------------------------------------------------------------------
 */




int zc_sha1(char *sha, const char *data, int datalen)
{
    zcSHA temp;

    zc_sha1_init(&temp);
    zc_sha1_update(&temp, (uint8_t*)data, datalen);
    zc_sha1_final(&temp, (unsigned char*)sha);

    return ZC_OK;
}

int zc_sha1_bcd(char *sha, const char *data, int datalen)
{
    char buf[SHA_DIGESTSIZE];
    zc_sha1(buf, data, datalen);
    return zc_bin2bcd(sha, buf, SHA_DIGESTSIZE);
}

int zc_sha1_base64(char *sha, const char *data, int datalen)
{
    char buf[SHA_DIGESTSIZE];
    zc_sha1(buf, data, datalen);
    return zc_base64_enc(sha, buf, SHA_DIGESTSIZE);
}



