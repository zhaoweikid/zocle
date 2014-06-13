#ifndef ZOCLE_STRBUF_H
#define ZOCLE_STRBUF_H

#include <stdio.h>
#include <stdint.h>
#include <zocle/str/string.h>

typedef struct zc_strbuf_t
{
	struct zc_strbuf_t *next;
    uint32_t pos;
	zcString *data;
}zcStrBuf;

zcStrBuf*   zc_strbuf_new(zcString *);
void        zc_strbuf_delete(void *);

int			zc_strbuf_init(zcStrBuf*, zcString*);
void        zc_strbuf_destroy(void *);

int         zc_strbuf_new2(zcStrBuf **, zcString *);

void        zc_strbuf_clear(zcStrBuf *);
int         zc_strbuf_set(zcStrBuf *, void *data, int len);
int         zc_strbuf_get(zcStrBuf *, void *data, int len);
int         zc_strbuf_append(zcStrBuf *, void *data, int len);
int			zc_strbuf_compact(zcStrBuf*);

#define     zc_strbuf_reset(buf)	zc_strbuf_clear
#define		zc_strbuf_len(buf)		(buf->data->len-buf->pos)
#define     zc_strbuf_idle(buf)		(buf->data->size-buf->data->len)
#define		zc_strbuf_data(buf)		&buf->data->data[buf->pos]
#define     zc_strbuf_delete_null(x) do{zc_strbuf_delete(x);x=NULL;}while(0)

#endif
