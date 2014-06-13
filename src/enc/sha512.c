#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <zocle/base/defines.h>
#include <zocle/enc/sha512.h>
#include <zocle/enc/bcd.h>
#include <zocle/enc/base64.h>


/* SHA512 module */

/* This module provides an interface to NIST's SHA-512 and SHA-384 Algorithms */

/* See below for information about the original code this module was
   based upon. Additional work performed by:

   Andrew Kuchling (amk@amk.ca)
   Greg Stein (gstein@lyra.org)
   Trevor Perrin (trevp@trevp.net)

   Copyright (C) 2005   Gregory P. Smith (greg@krypto.org)
   Licensed to PSF under a Contributor Agreement.

*/

/* Endianness testing and definitions */
#define TestEndianness(variable) {int i=1; variable=PCT_BIG_ENDIAN;\
        if (*((char*)&i)==1) variable=PCT_LITTLE_ENDIAN;}

#define PCT_LITTLE_ENDIAN 1
#define PCT_BIG_ENDIAN 0

/* Some useful types */

/* The SHA block size and message digest sizes, in bytes */

#define SHA_BLOCKSIZE   128
#define SHA_DIGESTSIZE  64

/* The structure for storing SHA info */

typedef struct {
    uint64_t digest[8];                /* Message digest */
    uint32_t count_lo, count_hi;       /* 64-bit bit count */
    uint8_t data[SHA_BLOCKSIZE];       /* SHA data buffer */
    int Endianness;
    int local;                          /* unprocessed amount in data */
    int digestsize;
} zcSHA512;

/* When run on a little-endian CPU we need to perform byte reversal on an
   array of longwords. */

static void longReverse(uint64_t *buffer, int byteCount, int Endianness)
{
    uint64_t value;

    if ( Endianness == PCT_BIG_ENDIAN )
        return;

    byteCount /= sizeof(*buffer);
    while (byteCount--) {
        value = *buffer;

                ((unsigned char*)buffer)[0] = (unsigned char)(value >> 56) & 0xff;
                ((unsigned char*)buffer)[1] = (unsigned char)(value >> 48) & 0xff;
                ((unsigned char*)buffer)[2] = (unsigned char)(value >> 40) & 0xff;
                ((unsigned char*)buffer)[3] = (unsigned char)(value >> 32) & 0xff;
                ((unsigned char*)buffer)[4] = (unsigned char)(value >> 24) & 0xff;
                ((unsigned char*)buffer)[5] = (unsigned char)(value >> 16) & 0xff;
                ((unsigned char*)buffer)[6] = (unsigned char)(value >>  8) & 0xff;
                ((unsigned char*)buffer)[7] = (unsigned char)(value      ) & 0xff;

                buffer++;
    }
}

/*void zc_sha512_copy(zcSHA512 *src, zcSHA512 *dest)
{
    dest->Endianness = src->Endianness;
    dest->local = src->local;
    dest->digestsize = src->digestsize;
    dest->count_lo = src->count_lo;
    dest->count_hi = src->count_hi;
    memcpy(dest->digest, src->digest, sizeof(src->digest));
    memcpy(dest->data, src->data, sizeof(src->data));
}*/


/* ------------------------------------------------------------------------
 *
 * This code for the SHA-512 algorithm was noted as public domain. The
 * original headers are pasted below.
 *
 * Several changes have been made to make it more compatible with the
 * Python environment and desired interface.
 *
 */

/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * gurantee it works.
 *
 * Tom St Denis, tomstdenis@iahu.ca, http://libtomcrypt.org
 */


/* SHA512 by Tom St Denis */

/* Various logical functions */
#define ROR64(x, y) \
    ( ((((x) & 0xFFFFFFFFFFFFFFFFU)>>((unsigned long long)(y) & 63)) | \
      ((x)<<((unsigned long long)(64-((y) & 63))))) & 0xFFFFFFFFFFFFFFFFU)
#define Ch(x,y,z)       (z ^ (x & (y ^ z)))
#define Maj(x,y,z)      (((x | y) & z) | (x & y))
#define S(x, n)         ROR64((x),(n))
#define R(x, n)         (((x) & 0xFFFFFFFFFFFFFFFFU) >> ((unsigned long long)n))
#define Sigma0(x)       (S(x, 28) ^ S(x, 34) ^ S(x, 39))
#define Sigma1(x)       (S(x, 14) ^ S(x, 18) ^ S(x, 41))
#define Gamma0(x)       (S(x, 1) ^ S(x, 8) ^ R(x, 7))
#define Gamma1(x)       (S(x, 19) ^ S(x, 61) ^ R(x, 6))


static void
zc_sha512_transform(zcSHA512 *sha_info)
{
    int i;
    uint64_t S[8], W[80], t0, t1;

    memcpy(W, sha_info->data, sizeof(sha_info->data));
    longReverse(W, (int)sizeof(sha_info->data), sha_info->Endianness);

    for (i = 16; i < 80; ++i) {
                W[i] = Gamma1(W[i - 2]) + W[i - 7] + Gamma0(W[i - 15]) + W[i - 16];
    }
    for (i = 0; i < 8; ++i) {
        S[i] = sha_info->digest[i];
    }

    /* Compress */
#define RND(a,b,c,d,e,f,g,h,i,ki)                    \
     t0 = h + Sigma1(e) + Ch(e, f, g) + ki + W[i];   \
     t1 = Sigma0(a) + Maj(a, b, c);                  \
     d += t0;                                        \
     h  = t0 + t1;

    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],0,0x428a2f98d728ae22U);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],1,0x7137449123ef65cdU);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],2,0xb5c0fbcfec4d3b2fU);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],3,0xe9b5dba58189dbbcU);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],4,0x3956c25bf348b538U);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],5,0x59f111f1b605d019U);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],6,0x923f82a4af194f9bU);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],7,0xab1c5ed5da6d8118U);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],8,0xd807aa98a3030242U);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],9,0x12835b0145706fbeU);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],10,0x243185be4ee4b28cU);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],11,0x550c7dc3d5ffb4e2U);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],12,0x72be5d74f27b896fU);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],13,0x80deb1fe3b1696b1U);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],14,0x9bdc06a725c71235U);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],15,0xc19bf174cf692694U);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],16,0xe49b69c19ef14ad2U);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],17,0xefbe4786384f25e3U);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],18,0x0fc19dc68b8cd5b5U);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],19,0x240ca1cc77ac9c65U);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],20,0x2de92c6f592b0275U);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],21,0x4a7484aa6ea6e483U);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],22,0x5cb0a9dcbd41fbd4U);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],23,0x76f988da831153b5U);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],24,0x983e5152ee66dfabU);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],25,0xa831c66d2db43210U);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],26,0xb00327c898fb213fU);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],27,0xbf597fc7beef0ee4U);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],28,0xc6e00bf33da88fc2U);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],29,0xd5a79147930aa725U);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],30,0x06ca6351e003826fU);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],31,0x142929670a0e6e70U);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],32,0x27b70a8546d22ffcU);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],33,0x2e1b21385c26c926U);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],34,0x4d2c6dfc5ac42aedU);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],35,0x53380d139d95b3dfU);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],36,0x650a73548baf63deU);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],37,0x766a0abb3c77b2a8U);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],38,0x81c2c92e47edaee6U);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],39,0x92722c851482353bU);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],40,0xa2bfe8a14cf10364U);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],41,0xa81a664bbc423001U);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],42,0xc24b8b70d0f89791U);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],43,0xc76c51a30654be30U);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],44,0xd192e819d6ef5218U);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],45,0xd69906245565a910U);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],46,0xf40e35855771202aU);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],47,0x106aa07032bbd1b8U);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],48,0x19a4c116b8d2d0c8U);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],49,0x1e376c085141ab53U);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],50,0x2748774cdf8eeb99U);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],51,0x34b0bcb5e19b48a8U);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],52,0x391c0cb3c5c95a63U);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],53,0x4ed8aa4ae3418acbU);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],54,0x5b9cca4f7763e373U);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],55,0x682e6ff3d6b2b8a3U);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],56,0x748f82ee5defb2fcU);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],57,0x78a5636f43172f60U);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],58,0x84c87814a1f0ab72U);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],59,0x8cc702081a6439ecU);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],60,0x90befffa23631e28U);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],61,0xa4506cebde82bde9U);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],62,0xbef9a3f7b2c67915U);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],63,0xc67178f2e372532bU);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],64,0xca273eceea26619cU);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],65,0xd186b8c721c0c207U);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],66,0xeada7dd6cde0eb1eU);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],67,0xf57d4f7fee6ed178U);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],68,0x06f067aa72176fbaU);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],69,0x0a637dc5a2c898a6U);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],70,0x113f9804bef90daeU);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],71,0x1b710b35131c471bU);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],72,0x28db77f523047d84U);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],73,0x32caab7b40c72493U);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],74,0x3c9ebe0a15c9bebcU);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],75,0x431d67c49c100d4cU);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],76,0x4cc5d4becb3e42b6U);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],77,0x597f299cfc657e2aU);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],78,0x5fcb6fab3ad6faecU);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],79,0x6c44198c4a475817U);

#undef RND

    /* feedback */
    for (i = 0; i < 8; i++) {
        sha_info->digest[i] = sha_info->digest[i] + S[i];
    }

}



/* initialize the SHA digest */

static void
zc_sha512_init(zcSHA512 *sha_info)
{
    TestEndianness(sha_info->Endianness)
    sha_info->digest[0] = 0x6a09e667f3bcc908U;
    sha_info->digest[1] = 0xbb67ae8584caa73bU;
    sha_info->digest[2] = 0x3c6ef372fe94f82bU;
    sha_info->digest[3] = 0xa54ff53a5f1d36f1U;
    sha_info->digest[4] = 0x510e527fade682d1U;
    sha_info->digest[5] = 0x9b05688c2b3e6c1fU;
    sha_info->digest[6] = 0x1f83d9abfb41bd6bU;
    sha_info->digest[7] = 0x5be0cd19137e2179U;
    sha_info->count_lo = 0L;
    sha_info->count_hi = 0L;
    sha_info->local = 0;
    sha_info->digestsize = 64;
}

static void
zc_sha384_init(zcSHA512 *sha_info)
{
    TestEndianness(sha_info->Endianness)
    sha_info->digest[0] = 0xcbbb9d5dc1059ed8U;
    sha_info->digest[1] = 0x629a292a367cd507U;
    sha_info->digest[2] = 0x9159015a3070dd17U;
    sha_info->digest[3] = 0x152fecd8f70e5939U;
    sha_info->digest[4] = 0x67332667ffc00b31U;
    sha_info->digest[5] = 0x8eb44a8768581511U;
    sha_info->digest[6] = 0xdb0c2e0d64f98fa7U;
    sha_info->digest[7] = 0x47b5481dbefa4fa4U;
    sha_info->count_lo = 0L;
    sha_info->count_hi = 0L;
    sha_info->local = 0;
    sha_info->digestsize = 48;
}


/* update the SHA digest */

static void
zc_sha512_update(zcSHA512 *sha_info, uint8_t *buffer, int count)
{
    int i;
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
            zc_sha512_transform(sha_info);
        }
        else {
            return;
        }
    }
    while (count >= SHA_BLOCKSIZE) {
        memcpy(sha_info->data, buffer, SHA_BLOCKSIZE);
        buffer += SHA_BLOCKSIZE;
        count -= SHA_BLOCKSIZE;
        zc_sha512_transform(sha_info);
    }
    memcpy(sha_info->data, buffer, count);
    sha_info->local = count;
}

/* finish computing the SHA digest */

static void
zc_sha512_final(zcSHA512 *sha_info, unsigned char digest[SHA_DIGESTSIZE])
{
    int count;
    uint32_t lo_bit_count, hi_bit_count;

    lo_bit_count = sha_info->count_lo;
    hi_bit_count = sha_info->count_hi;
    count = (int) ((lo_bit_count >> 3) & 0x7f);
    ((uint8_t *) sha_info->data)[count++] = 0x80;
    if (count > SHA_BLOCKSIZE - 16) {
        memset(((uint8_t *) sha_info->data) + count, 0,
               SHA_BLOCKSIZE - count);
        zc_sha512_transform(sha_info);
        memset((uint8_t *) sha_info->data, 0, SHA_BLOCKSIZE - 16);
    }
    else {
        memset(((uint8_t *) sha_info->data) + count, 0,
               SHA_BLOCKSIZE - 16 - count);
    }

    /* GJS: note that we add the hi/lo in big-endian. zc_sha512_transform will
       swap these values into host-order. */
    sha_info->data[112] = 0;
    sha_info->data[113] = 0;
    sha_info->data[114] = 0;
    sha_info->data[115] = 0;
    sha_info->data[116] = 0;
    sha_info->data[117] = 0;
    sha_info->data[118] = 0;
    sha_info->data[119] = 0;
    sha_info->data[120] = (hi_bit_count >> 24) & 0xff;
    sha_info->data[121] = (hi_bit_count >> 16) & 0xff;
    sha_info->data[122] = (hi_bit_count >>  8) & 0xff;
    sha_info->data[123] = (hi_bit_count >>  0) & 0xff;
    sha_info->data[124] = (lo_bit_count >> 24) & 0xff;
    sha_info->data[125] = (lo_bit_count >> 16) & 0xff;
    sha_info->data[126] = (lo_bit_count >>  8) & 0xff;
    sha_info->data[127] = (lo_bit_count >>  0) & 0xff;
    zc_sha512_transform(sha_info);
    digest[ 0] = (unsigned char) ((sha_info->digest[0] >> 56) & 0xff);
    digest[ 1] = (unsigned char) ((sha_info->digest[0] >> 48) & 0xff);
    digest[ 2] = (unsigned char) ((sha_info->digest[0] >> 40) & 0xff);
    digest[ 3] = (unsigned char) ((sha_info->digest[0] >> 32) & 0xff);
    digest[ 4] = (unsigned char) ((sha_info->digest[0] >> 24) & 0xff);
    digest[ 5] = (unsigned char) ((sha_info->digest[0] >> 16) & 0xff);
    digest[ 6] = (unsigned char) ((sha_info->digest[0] >>  8) & 0xff);
    digest[ 7] = (unsigned char) ((sha_info->digest[0]      ) & 0xff);
    digest[ 8] = (unsigned char) ((sha_info->digest[1] >> 56) & 0xff);
    digest[ 9] = (unsigned char) ((sha_info->digest[1] >> 48) & 0xff);
    digest[10] = (unsigned char) ((sha_info->digest[1] >> 40) & 0xff);
    digest[11] = (unsigned char) ((sha_info->digest[1] >> 32) & 0xff);
    digest[12] = (unsigned char) ((sha_info->digest[1] >> 24) & 0xff);
    digest[13] = (unsigned char) ((sha_info->digest[1] >> 16) & 0xff);
    digest[14] = (unsigned char) ((sha_info->digest[1] >>  8) & 0xff);
    digest[15] = (unsigned char) ((sha_info->digest[1]      ) & 0xff);
    digest[16] = (unsigned char) ((sha_info->digest[2] >> 56) & 0xff);
    digest[17] = (unsigned char) ((sha_info->digest[2] >> 48) & 0xff);
    digest[18] = (unsigned char) ((sha_info->digest[2] >> 40) & 0xff);
    digest[19] = (unsigned char) ((sha_info->digest[2] >> 32) & 0xff);
    digest[20] = (unsigned char) ((sha_info->digest[2] >> 24) & 0xff);
    digest[21] = (unsigned char) ((sha_info->digest[2] >> 16) & 0xff);
    digest[22] = (unsigned char) ((sha_info->digest[2] >>  8) & 0xff);
    digest[23] = (unsigned char) ((sha_info->digest[2]      ) & 0xff);
    digest[24] = (unsigned char) ((sha_info->digest[3] >> 56) & 0xff);
    digest[25] = (unsigned char) ((sha_info->digest[3] >> 48) & 0xff);
    digest[26] = (unsigned char) ((sha_info->digest[3] >> 40) & 0xff);
    digest[27] = (unsigned char) ((sha_info->digest[3] >> 32) & 0xff);
    digest[28] = (unsigned char) ((sha_info->digest[3] >> 24) & 0xff);
    digest[29] = (unsigned char) ((sha_info->digest[3] >> 16) & 0xff);
    digest[30] = (unsigned char) ((sha_info->digest[3] >>  8) & 0xff);
    digest[31] = (unsigned char) ((sha_info->digest[3]      ) & 0xff);
    digest[32] = (unsigned char) ((sha_info->digest[4] >> 56) & 0xff);
    digest[33] = (unsigned char) ((sha_info->digest[4] >> 48) & 0xff);
    digest[34] = (unsigned char) ((sha_info->digest[4] >> 40) & 0xff);
    digest[35] = (unsigned char) ((sha_info->digest[4] >> 32) & 0xff);
    digest[36] = (unsigned char) ((sha_info->digest[4] >> 24) & 0xff);
    digest[37] = (unsigned char) ((sha_info->digest[4] >> 16) & 0xff);
    digest[38] = (unsigned char) ((sha_info->digest[4] >>  8) & 0xff);
    digest[39] = (unsigned char) ((sha_info->digest[4]      ) & 0xff);
    digest[40] = (unsigned char) ((sha_info->digest[5] >> 56) & 0xff);
    digest[41] = (unsigned char) ((sha_info->digest[5] >> 48) & 0xff);
    digest[42] = (unsigned char) ((sha_info->digest[5] >> 40) & 0xff);
    digest[43] = (unsigned char) ((sha_info->digest[5] >> 32) & 0xff);
    digest[44] = (unsigned char) ((sha_info->digest[5] >> 24) & 0xff);
    digest[45] = (unsigned char) ((sha_info->digest[5] >> 16) & 0xff);
    digest[46] = (unsigned char) ((sha_info->digest[5] >>  8) & 0xff);
    digest[47] = (unsigned char) ((sha_info->digest[5]      ) & 0xff);
    digest[48] = (unsigned char) ((sha_info->digest[6] >> 56) & 0xff);
    digest[49] = (unsigned char) ((sha_info->digest[6] >> 48) & 0xff);
    digest[50] = (unsigned char) ((sha_info->digest[6] >> 40) & 0xff);
    digest[51] = (unsigned char) ((sha_info->digest[6] >> 32) & 0xff);
    digest[52] = (unsigned char) ((sha_info->digest[6] >> 24) & 0xff);
    digest[53] = (unsigned char) ((sha_info->digest[6] >> 16) & 0xff);
    digest[54] = (unsigned char) ((sha_info->digest[6] >>  8) & 0xff);
    digest[55] = (unsigned char) ((sha_info->digest[6]      ) & 0xff);
    digest[56] = (unsigned char) ((sha_info->digest[7] >> 56) & 0xff);
    digest[57] = (unsigned char) ((sha_info->digest[7] >> 48) & 0xff);
    digest[58] = (unsigned char) ((sha_info->digest[7] >> 40) & 0xff);
    digest[59] = (unsigned char) ((sha_info->digest[7] >> 32) & 0xff);
    digest[60] = (unsigned char) ((sha_info->digest[7] >> 24) & 0xff);
    digest[61] = (unsigned char) ((sha_info->digest[7] >> 16) & 0xff);
    digest[62] = (unsigned char) ((sha_info->digest[7] >>  8) & 0xff);
    digest[63] = (unsigned char) ((sha_info->digest[7]      ) & 0xff);
}



/*
 * End of copied SHA code.
 *
 * ------------------------------------------------------------------------
 */

int
zc_sha512(char *sha, const char *data, int datalen)
{
    zcSHA512 temp;

    zc_sha512_init(&temp);
    zc_sha512_update(&temp, (uint8_t*)data, datalen);
    zc_sha512_final(&temp, (unsigned char*)sha);

    return ZC_OK;
}

int zc_sha512_bcd(char *sha, const char *data, int datalen)
{
    char buf[SHA_DIGESTSIZE];
    zc_sha512(buf, data, datalen);
    return zc_bin2bcd(sha, buf, sizeof(buf));
}

int zc_sha512_base64(char *sha, const char *data, int datalen)
{
    char buf[SHA_DIGESTSIZE];
    zc_sha512(buf, data, datalen);
    return zc_base64_enc(sha, (char*)buf, SHA_DIGESTSIZE);
}

int
zc_sha384(char *sha, const char *data, int datalen)
{
    zcSHA512 temp;

    zc_sha384_init(&temp);
    zc_sha512_update(&temp, (uint8_t*)data, datalen);
    zc_sha512_final(&temp, (unsigned char*)sha);

    return ZC_OK;
}

int zc_sha384_bcd(char *sha, const char *data, int datalen)
{
    char buf[SHA_DIGESTSIZE];
    zc_sha384(buf, data, datalen);
    return zc_bin2bcd(sha, buf, 48);
}

int zc_sha384_base64(char *sha, const char *data, int datalen)
{
    char buf[SHA_DIGESTSIZE];
    zc_sha384(buf, data, datalen);
    return zc_base64_enc(sha, buf, 48);
}

