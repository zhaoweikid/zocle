/*  base64 coding (3/4 relation)
 *  according on RFC 1421
 */
#include <stdio.h>
#include <zocle/enc/base64.h>
#include <zocle/base/defines.h>

#define BASE64_CICLES_PERLINE       18
#define BASE64_LINEWIDTH        (BASE64_CICLES_PERLINE*4)
#define BASE64_INPUT_LENGTH_PER_LINE    (BASE64_CICLES_PERLINE*3)
#define BASE64_ALIGN_INPUT_SIZE(size)   ((size) + ((size) % BASE64_INPUT_LENGTH_PER_LINE ? BASE64_INPUT_LENGTH_PER_LINE - ((size) % BASE64_INPUT_LENGTH_PER_LINE) : 0))

size_t base64encsz(size_t datasz);
size_t base64padsz(size_t datasz);
size_t base64decsz(size_t srcsz);
size_t base64realdecsz(unsigned char *input, size_t inputsz);



#define USE_BASE64_DECODE_TABLE

static char *base64digits =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char base64pad = '=';

#ifdef USE_BASE64_DECODE_TABLE

static char base64dectable[256] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
	-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
	-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

#endif

size_t base64encsz(size_t datasz)
{
    size_t sz = ((datasz + 2) / 3) * 4;
    /* number of lines * 2, MAY be CRLF */
    size_t nln = ((sz + (BASE64_LINEWIDTH-1)) / BASE64_LINEWIDTH) * 2;
    return sz + nln;
}

size_t base64padsz(size_t datasz)
{
    size_t sz1 = datasz % 3;
    return 3 - (sz1 ? sz1 : 3);
}

/* consider pure base64 digits
 */
size_t base64decsz(size_t srcsz)
{
    return srcsz * 3 / 4;
}

size_t base64realdecsz(unsigned char *input, size_t inputsz)
{
	size_t rem;
	size_t count = 0;
	while (inputsz--) if (base64dectable[*input++] != -1) ++count;
	rem = count % 4;
	if (rem == 1) {
		fprintf(stderr, "gcslib: base64realdecsz(): incorrect input size: %lu\n", count);
		return 0;
	}
	count = count / 4 * 3 + (rem ? rem - 1 : 0);
	return count;
}

int zc_base64_enc_str(unsigned char *output, void *input, size_t inputsz, int multiline)
{
    int div = inputsz / 3;
    int rem = inputsz % 3;
    int i;

    register unsigned char *src = (unsigned char *) input;
    register unsigned char *buf = (unsigned char *) output;

    i = 0;

    while (div-- > 0) {
        buf[0] = base64digits[(src[0] >> 2) & 0x3f];
        buf[1] = base64digits[((src[0] << 4) & 0x30) + ((src[1] >> 4) & 0xf)];
        buf[2] = base64digits[((src[1] << 2) & 0x3c) + ((src[2] >> 6) & 0x3)];
        buf[3] = base64digits[src[2] & 0x3f];
        src += 3;

        if (multiline == ZC_TRUE) {
            if (++i == BASE64_CICLES_PERLINE) {
                i = 0;
                buf[4] = '\n';
                buf += 5;
            } else {
                buf += 4;
            }
        }else{
            buf += 4;
        }
    }

    switch (rem) {
    case 2:
	buf[0] = base64digits[(src[0] >> 2) & 0x3f];
	buf[1] = base64digits[((src[0] << 4) & 0x30) + ((src[1] >> 4) & 0xf)];
	buf[2] = base64digits[(src[1] << 2) & 0x3c];
	buf[3] = base64pad;
	buf += 4;
	break;
    case 1:
	buf[0] = base64digits[(src[0] >> 2) & 0x3f];
	buf[1] = base64digits[(src[0] << 4) & 0x30];
	buf[2] = buf[3] = base64pad;
	buf += 4;
	break;
    }

    *buf = 0;
    return buf - output;
}

int zc_base64_enc(char *output, const char *input, int inputsz)
{
    return zc_base64_enc_str((unsigned char*)output, (void*)input, (size_t)inputsz, ZC_FALSE);
}

int zc_base64_enc_multiline(char *output, const char *input, int inputsz)
{
    return zc_base64_enc_str((unsigned char*)output, (void*)input, (size_t)inputsz, ZC_TRUE);
}



/*
void base64gendectable()
{
	int i, j;

	//ZINFO("static char base64dectable[256] = {\n\t");

	for (i=0; i<255;) {

	    if ((i >= 'A') && (i <= 'Z')) j = i - 'A';
	    else if ((i >= 'a') && (i <= 'z')) j = i + 26 - 'a';
	    else if ((i >= '0') && (i <= '9')) j = i + 52 - '0';
	    else if (i == '+') j = 62;
	    else if (i == '/') j = 63;
	    else j = -1;

	    if (!(++i%16)) {
		if (j >=0 && j < 10){ 
            //ZINFO(" %i,\n\t", j);
        }else{ 
            //ZINFO("%02i,\n\t", j);
	    } 
        }else {
	    	if (j >=0 && j < 10){
                //ZINFO(" %i, ", j);
            }else{ 
                //ZINFO("%02i, ", j);
            }
	    }
	}
	puts("-1\n};");
}*/

int zc_base64_dec2(void* output, unsigned char *input, size_t inputsz, unsigned char *discarded)
{
    register unsigned char uch = 0, *buf = output;
    unsigned char in[4] = {0,0,0,0};
    int i, j = inputsz;

    if (discarded) *discarded = 0;

    for (;;) {
	i = 0;
	while (i < 4) {

	    if ( ! j-- ) {
		++j;
		if (i == 1 && discarded)
			*discarded = base64digits[in[0]];
		break;
	    }

	    uch = *(input++);

#ifndef USE_BASE64_DECODE_TABLE

	    if ((uch >= 'A') && (uch <= 'Z')) in[i] = (uch - 'A');
	    else if ((uch >= 'a') && (uch <= 'z')) in[i] = (uch - 'a' + 26);
	    else if ((uch >= '0') && (uch <= '9')) in[i] = (uch - '0' + 52);
	    else if (uch == '+') in[i] = 62;
	    else if (uch == '/') in[i] = 63;
	    else continue;	/* ignore all non-base64 characters */

#else

	    if ((char)(uch = base64dectable[uch]) != -1) in[i] = uch;
	    else continue;

#endif
	    ++i;
	}

	if ( i != 4 ) break;

	/* pre is faster than pos */
	*(  buf) = ((in[0] << 2) & 0xfc) | ((in[1] >> 4) & 0x03);
	*(++buf) = ((in[1] << 4) & 0xf0) | ((in[2] >> 2) & 0x0f);
	*(++buf) = ((in[2] << 6) & 0xc0) | (in[3] & 0x3f);
	++buf;
    }

    j = 0;

    switch (i) {
    	case 3: buf[1] = ((in[1] << 4) & 0xf0) | ((in[2] >> 2) & 0x0f); ++j;
    	case 2: buf[0] = ((in[0] << 2) & 0xfc) | ((in[1] >> 4) & 0x03); ++j;
    }

    buf += j;

    return buf - (unsigned char*) output;
}

int zc_base64_dec(char* output, const char *input, int inputsz)
{
    register unsigned char uch = 0, *buf = (unsigned char*)output;
    unsigned char in[4] = {0,0,0,0};
    int i, j = inputsz;

    //if (discarded) *discarded = 0;

    for (;;) {
	i = 0;
	while (i < 4) {

	    if ( ! j-- ) {
		++j;
		//if (i == 1 && discarded)
	    //		*discarded = base64digits[in[0]];
		break;
	    }

	    uch = *(input++);

#ifndef USE_BASE64_DECODE_TABLE

	    if ((uch >= 'A') && (uch <= 'Z')) in[i] = (uch - 'A');
	    else if ((uch >= 'a') && (uch <= 'z')) in[i] = (uch - 'a' + 26);
	    else if ((uch >= '0') && (uch <= '9')) in[i] = (uch - '0' + 52);
	    else if (uch == '+') in[i] = 62;
	    else if (uch == '/') in[i] = 63;
	    else continue;	/* ignore all non-base64 characters */

#else

	    if ((char)(uch = base64dectable[uch]) != -1) in[i] = uch;
	    else continue;

#endif
	    ++i;
	}

	if ( i != 4 ) break;

	/* pre is faster than pos */
	*(  buf) = ((in[0] << 2) & 0xfc) | ((in[1] >> 4) & 0x03);
	*(++buf) = ((in[1] << 4) & 0xf0) | ((in[2] >> 2) & 0x0f);
	*(++buf) = ((in[2] << 6) & 0xc0) | (in[3] & 0x3f);
	++buf;
    }

    j = 0;

    switch (i) {
    	case 3: buf[1] = ((in[1] << 4) & 0xf0) | ((in[2] >> 2) & 0x0f); ++j;
    	case 2: buf[0] = ((in[0] << 2) & 0xfc) | ((in[1] >> 4) & 0x03); ++j;
    }

    buf += j;

    return buf - (unsigned char*)output;
}

