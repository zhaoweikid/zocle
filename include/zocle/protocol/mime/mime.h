#ifdef ZOCLE_WITH_ICONV
#ifndef ZOCLE_MIME_MIME_H
#define ZOCLE_MIME_MIME_H

#include <stdio.h>
#include <zocle/ds/dict.h>
#include <zocle/ds/list.h>
#include <zocle/str/string.h>

int zc_mimeheadext_encode(zcString *f, const char *fchar, zcString *t, const char *tchar, char tenc);
int zc_mimeheadext_decode(zcString *f, zcString *t, const char *tchar);

typedef struct zc_mimetype_t {
	zcString maintype;
	zcString subtype;
	zcString boundary;
	zcString charset;
	zcDict  *pairs;
}zcMimeType;

zcMimeType* zc_mimetype_new();
void		zc_mimetype_delete(void*);
int			zc_mimetype_init(zcMimeType *);
void		zc_mimetype_destroy(void *);


typedef struct zc_mimeheader_t {
	zcDict      *headers;
	zcMimeType   ctype;
}zcMimeHeader;

int  zc_mimeheader_init(zcMimeHeader *);
void zc_mimeheader_destroy(void *);
int  zc_mimeheader_parse(zcMimeHeader*);


typedef struct zc_mimebody_t {
	zcString *name;
	zcString *data;
	int		  offset;
	int	      len;
}zcMimeBody;

int  zc_mimebody_init(zcMimeBody *);
void zc_mimebody_destroy(void *);


typedef struct zc_mimepart_t {
	zcMimeHeader header;
	zcMimeBody	 body;
	zcList		*children;
}zcMimePart;

zcMimePart*	zc_mimepart_new();
void		zc_mimepart_delete(void *);
int			zc_mimepart_init(zcMimePart*);
void		zc_mimepart_destroy(void *);


/*typedef struct zc_mime_t {
	zcMimeHeader header;
	zcList		*part;
}zcMime;

zcMime* zc_mime_new();
void	zc_mime_delete(void *);	
int		zc_mime_init(zcMime*);
void	zc_mime_destroy(void *);*/


#endif
#endif
