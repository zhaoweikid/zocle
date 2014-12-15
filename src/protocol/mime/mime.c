#ifdef ZOCLE_WITH_ICONV
#include <zocle/protocol/mime/mime.h>
#include <zocle/enc/base64.h>
#include <zocle/enc/qp.h>
#include <zocle/str/convert.h>
#include <zocle/str/string.h>
#include <zocle/str/cstring.h>

int 
zc_mimeheadext_encode(zcString *f, const char *fchar, zcString *t, const char *tchar, char tenc)
{
    if (tenc > 'Z')
        tenc -= 32;
    if (tenc != 'B' && tenc != 'Q')  // must base64 or quoted-printable
        return ZC_ERR;
  
    zcString *tdata = NULL;
    if (strcmp(fchar, tchar) != 0) {
        tdata = zc_str_convert(f, fchar, tchar); 
        if (NULL == tdata) {
            return ZC_ERR;
        }
    }
    int ret, retcode = ZC_OK;
    zcString *tdata2 = zc_str_new(tdata->len*3+1);
    if (tenc == 'B') {
        ret = zc_base64_enc(tdata2->data, tdata->data, tdata->len);
    }else{
        ret = zc_qp_enc(tdata2->data, tdata->data, tdata->len);
    }
    if (ret < 0) {
        retcode = ZC_ERR;
        goto encode_over;
    }
    tdata2->len = ret;

    zc_str_append_format(t, "=?%s?%s?%s?=", tchar, tenc, tdata2->data);
encode_over:
    zc_str_delete(tdata);
    zc_str_delete(tdata2);
    return retcode;
}

int 
zc_mimeheadext_decode(zcString *f, zcString *t, const char *tchar)
{
    const char *p, *start = f->data;
    const char *end = f->data + f->len;
    int  oldlen = t->len;
    char charset[64] = {0};
    char tenc = 0;
    int  i, ret;
    zcString *tdata = NULL;

    while (start < end) {
        p = strstr(start, "=?");
        if (p == NULL) {
            zc_str_append_len(t, start, end-start);
            return ZC_OK;
        }
        if (p - start > 0) {
            zc_str_append_len(t, start, p-start);
        }
        i = 0; 
        p += 2;  // skip =?
        while (*p && *p != '?') {
            charset[i] = *p;
            i++;
            p++;
            if (i == 64) {
                goto decode_error;
            }
        }
        charset[i] = 0;
        if (*p != '?') {
            goto decode_error;
        }
        p++; // skip ?
        tenc = *p;
        if (tenc > 'Z') {
            tenc -= 32;
        }
        p++; // skip ?
        if (*p != '?' || (tenc != 'B' && tenc != 'Q' && tenc != 'b' && tenc != 'q')) {
            goto decode_error;
        }
        const char *data = p;
        const char *p1 = strstr(p, "?=");
        if (p1 == NULL) {
            goto decode_error;
        }
        int datalen = p1 - p;
        
        if (tdata) {
            zc_str_ensure_idle_size(tdata, datalen+1);
        }else{
            tdata = zc_str_new(datalen+1);
        }
        if (tenc == 'B') {
            ret = zc_base64_dec(tdata->data, data, datalen);
        }else{
            ret = zc_qp_dec(tdata->data, data, datalen);
        }
        if (ret < 0) {
            goto decode_error;
        }
        tdata->len = ret;
        
        zc_str_ensure_idle_size(t, tdata->len);
        ret = zc_iconv_convert(charset, tchar, tdata->data, tdata->len, &t->data[t->len], tdata->len);
        if (ret < 0) {
            goto decode_error;
        }
        t->len += ret;

        p += 2; // skip ?=

        while (*p && (*p == ' ' || *p == '\t')) p++;
    }

    if (tdata) {
        zc_str_delete(tdata);
    }
    return ZC_OK;

decode_error:
    zc_str_truncate(t, oldlen);
    if (tdata) {
        zc_str_delete(tdata);
    }
    return ZC_ERR;
}

zcMimeType* 
zc_mimetype_new()
{
    zcMimeType *mt = (zcMimeType*)zc_calloc(sizeof(zcMimeType));
    zc_mimetype_init(mt);
    return mt;
}

void
zc_mimetype_delete(void *x)
{
    zc_mimetype_destroy(x);
    zc_free(x);
}

int
zc_mimetype_init(zcMimeType *mt)
{
    zc_str_init(&mt->maintype, 0);
    zc_str_init(&mt->subtype, 0);
    zc_str_init(&mt->boundary, 0);
    zc_str_init(&mt->charset, 0);

    mt->pairs = zc_dict_new_full(128, 0, zc_free_func, zc_free_func);
    return ZC_OK;
}

void 
zc_mimetype_destroy(void *x)
{
    zcMimeType *t = (zcMimeType*)x;
    zc_str_destroy(&t->maintype);
    zc_str_destroy(&t->subtype);
    zc_str_destroy(&t->boundary);
    zc_str_destroy(&t->charset);
    zc_dict_delete(t->pairs);
}

int 
zc_mimeheader_init(zcMimeHeader *header)
{
    header->headers = zc_dict_new_full(128, 0, zc_free_func, zc_free_func);
    return zc_mimetype_init(&header->ctype);
}

void 
zc_mimeheader_destroy(void *x)
{
    zcMimeHeader *header = (zcMimeHeader*)x;
    zc_dict_delete(header->headers);
    zc_mimetype_destroy(&header->ctype);
}

static int
zc_mimeheader_parse_contenttype(zcMimeHeader *header, zcString *s)
{
    return ZC_OK;
}


int 
zc_mimeheader_parse(zcMimeHeader *header)
{
    zcDict *h = header->headers; 
    
    zcString *ctype = zc_dict_get_str(h, "content-type", NULL);
    if (NULL == ctype) {
        ZCNOTE("not found content-type!");
        return 0;
    }
    return zc_mimeheader_parse_contenttype(header, ctype);
}

int
zc_mimebody_init(zcMimeBody *body)
{
    memset(body, 0, sizeof(zcMimeBody));
    return ZC_OK;
}

void 
zc_mimebody_destroy(void *x)
{
}



zcMimePart*	
zc_mimepart_new()
{
    zcMimePart *mp = (zcMimePart*)zc_malloc(sizeof(zcMimePart));
    zc_mimepart_init(mp);
    return mp;
}
void		
zc_mimepart_delete(void *x)
{
    zc_mimepart_destroy(x);
    zc_free(x);
}

int			
zc_mimepart_init(zcMimePart *m)
{
    zc_mimeheader_init(&m->header);
    zc_mimebody_init(&m->body);
    return ZC_OK;
}

void		
zc_mimepart_destroy(void *x)
{
    zcMimePart *m = (zcMimePart*)x;
    zc_mimeheader_destroy(&m->header);
    zc_mimebody_destroy(&m->body);
}

// -------------
/*
zcMime*  
zc_mime_new()
{
    zcMime *m = (zcMime*)zc_calloc(sizeof(zcMime));
    zc_mime_init(m);
    return m;
}

void		
zc_mime_delete(void *x)
{
    zc_mime_destroy(x);
    zc_free(x);
}

int	
zc_mime_init(zcMime *m)
{
    m->header = zc_dict_new_full(128, 0, zc_free_func, zc_free_func);
    m->part  = zc_list_new();
    m->part->del = zc_mimepart_delete;
    return ZC_OK;
}

void
zc_mime_destroy(void *x)
{
    zcMime *m = (zcMime*)x;

    zc_dict_delete(m->header);
    zc_list_delete(m->part);
}
*/

static int
zc_mimepart_parse_header(zcMimePart *m, zcString *s)
{
    //char *line = NULL;
    int  pos;

    zcCString *key   = zc_cstr_alloc_stack(128);
    zc_cstr_init(key, 128);

    zcString  *value = NULL;//zc_str_new(1024);

    while (1) {
        pos = zc_str_find(s, "\r\n");
        if (pos == 0) { // header end
            s->data += 2;
            s->len  -= 2;
            s->size -= 2;
            return 0;
        }

        if (pos < 0) 
            pos = s->len;
        /*if (pos < 0) {
            ZCNOTE("not found \\r\\n! %10s", s->data);
            return ZC_ERR;
        }*/

        if (s->data[0] == ' ' || s->data[0] == '\t') {
            int i = 0;
            while (s->data[i] == ' ' || s->data[i] == '\t') i++;

            if (value) {
                zc_str_append_len(value, &s->data[i], pos-i);
            }
            
            s->data += pos+2;
            s->len  -= pos+2;
            s->size -= pos+2;
 
            continue;
        }
      
        zc_cstr_clear(key);
        int i = 0;
        while (s->data[i] && s->data[i] != ':' && i<pos) {
            zc_cstr_append_c(key, s->data[i]);
            i++;
        }
        zc_cstr_low(key);
        ZCINFO("key:%s", key->data);
        
        if (s->data[i] != ':') {
            ZCNOTE("key error! %10s", s->data);
            return ZC_ERR;
        }
        i++;
        while (s->data[i] == ' ' || s->data[i] == '\t') i++;

        value = zc_str_new(32);
        zc_str_append_len(value, &s->data[i], pos-i);  

        ZCINFO("value:%s", value->data);

        zcObject *x = zc_dict_get_str(m->header.headers, key->data, NULL);
        if (x != NULL) {
            if (x->__type == ZC_STRING) {
                zcList *list = zc_list_new();
                zc_list_append(list, x);
                zc_list_append(list, value);
                zc_dict_set_str(m->header.headers, key->data, list);
            }else{
                zcList *list = (zcList*)x;
                zc_list_append(list, value);
            }
        }else{
            zc_dict_add_str(m->header.headers, key->data, value);
        }

        s->data += pos+2;
        s->len  -= pos+2;
        s->size -= pos+2;
    }
}

static int
zc_mimepart_parse_body(zcMimePart *m, zcString *s)
{
    zcMimeType *t = &m->header.ctype;
    if (t->boundary.len == 0) { // no boundary
    }else{
    }

    return ZC_OK;
}

int 
zc_mimepart_parse_str(zcMimePart *m, zcString *input)
{
    zcString s;
    zc_str_init_stack_exist(&s, input->data, input->len);

    int ret = zc_mimepart_parse_header(m, &s);
    if (ret < 0)
        return ZC_ERR;

    ret = zc_mimepart_parse_body(m, &s);
    if (ret < 0) {
        return ZC_ERR;
    }
    return ZC_OK;
}


#endif
