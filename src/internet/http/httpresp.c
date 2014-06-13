#include <zocle/internet/http/httpresp.h>
#include <zocle/mem/alloc.h>
#include <zocle/ds/dict.h>
#include <zocle/str/cstring.h>
#include <zocle/str/string.h>
#include <ctype.h>
#include <zocle/internet/http/httpheader.h>
#include <zocle/internet/http/httpreq.h>
#include <zocle/enc/sha1.h>

zcHttpResp* 
zc_httpresp_new()
{
    zcHttpResp *resp = (zcHttpResp*)zc_malloc(sizeof(zcHttpResp));
    memset(resp, 0, sizeof(zcHttpResp));
    
    zc_str_init(&resp->headdata, 0);
    zc_str_init(&resp->bodydata, 0);
    zc_str_init(&resp->protocol, 5);
    zc_str_init(&resp->msg, 0);

    resp->header = zc_dict_new_full(256, 0, zc_free_func, zc_free_func);
    resp->cookie = zc_dict_new_full(128, 0, zc_free_func, zc_cookie_delete);

    return resp;
}

void		
zc_httpresp_delete(void *x)
{
    zcHttpResp *resp = (zcHttpResp*)x;
    zc_str_destroy3(&resp->headdata, __FILE__, __LINE__);
    zc_str_destroy3(&resp->bodydata, __FILE__, __LINE__);
    zc_str_destroy3(&resp->protocol, __FILE__, __LINE__);
    zc_str_destroy3(&resp->msg, __FILE__, __LINE__);

    zc_dict_delete(resp->header);
    zc_dict_delete(resp->cookie);
    zc_free(resp);
}

int	
zc_httpresp_parse_header(zcHttpResp *resp)
{
    if (resp->headdata.len == 0)
        return ZC_ERR;

    zcCString *key = zc_cstr_alloc_stack(256);
    zc_cstr_init(key, 256);
    zcString value;
    
    zc_str_init(&value, 128);
  
    const char *start = resp->headdata.data;
    // parse first line:  HTTP/1.1 200 OK
    zcCString *s = zc_cstr_alloc_stack(32); 
    zc_cstr_init(s, 32); 

    while (*start && *start != '/') start++;
    if (*start == 0) return ZC_ERR;
    start++; // skip /
    
    while (*start && *start != ' ') {
        zc_cstr_append_c(s, *start);
        start++;
    }
    zc_str_assign(&resp->protocol, s->data, s->len);
    if (*start == 0) return ZC_ERR;
    while (*start && *start == ' ') start++;

    zc_cstr_clear(s);
    
    while (*start && isdigit(*start)) {
        zc_cstr_append_c(s, *start);
        start++;
    }
    resp->code = atoi(s->data);
    if (*start != ' ') return ZC_ERR;
    while (*start && *start == ' ') start++;
    
    while (*start && *start != '\r' && *start != '\n') {
        zc_str_append_c(&resp->msg, *start);
        start++;
    }
    while (*start && (*start == '\r' || *start == '\n')) start++;

    ZCINFO("http protocol:%s code:%d msg:%s", resp->protocol.data, resp->code, resp->msg.data);
    while (*start) {
        while (*start && *start != ':') {
            zc_cstr_append_c(key, *start);
            start++;
        }
        if (*start == ':') start++; // skip :
        while (*start && isblank(*start)) start++;

        while (*start && *start != '\r') {
            zc_str_append_c(&value, *start);
            start++;
        }
        while (*start && (*start == '\r' || *start == '\n')) start++;

        while (*start && isblank(*(start+1))) { // go ahead
            while (*start && isblank(*start)) start++; // skip blank
            while (*start && *start != '\r') {
                zc_str_append_c(&value, *start);
                start++;
            }
            while (*start && (*start == '\r' || *start == '\n')) start++;
        }

        ZCINFO("key:%s value:%s", key->data, value.data);
        
        if (strcmp(key->data, "Content-Length") == 0) {
            resp->bodylen = strtoll(value.data, NULL, 10);
        }else if (strcmp(key->data, "Content-Encoding") == 0) {
            if (strcasecmp(value.data, "gzip") == 0 || strcasecmp(value.data, "x-gzip") == 0) {
                resp->compress = ZC_HTTP_GZIP;
            }else if (strcmp(value.data, "compress") == 0) {
                resp->compress = ZC_HTTP_COMPRESS;
            }else if (strcmp(value.data, "deflate") == 0) {
                resp->compress = ZC_HTTP_DEFLATE;
            }
        }else if (strcmp(key->data, "Connection") == 0) {
            if (strcasecmp(value.data, "close") == 0) {
                resp->keepalive = 0;
            }else if (strcasecmp(value.data, "Keep-Alive") == 0) {
                resp->keepalive = 1;
            }
        }else if (strcmp(key->data, "Transfer-Encoding") == 0) {
            if (strcasecmp(value.data, "chunked") == 0)
                resp->chunked = 1;
        }else if (strcmp(key->data, "Set-Cookie") == 0 || strcmp(key->data, "Set-Cookie2") == 0) {
            // parse cookie 
            zcCookie *c = zc_cookie_new(value.data);
            if (c) {
                zc_dict_add(resp->cookie, c->key.data, 0, c);
                zc_cookie_print(c);
            }
        }

        zc_dict_add(resp->header, key->data, 0, zc_strdup(value.data, 0));
        
        zc_cstr_clear(key);
        zc_str_clear(&value);
    }

    zc_str_destroy(&value);
    return ZC_OK;
}

int	
zc_httpresp_parse(zcHttpResp *resp)
{
    int ret = zc_httpresp_parse_header(resp);
    if (ret < 0)
        return ret;
    // FIXME: pase body, maybe mime
    return ZC_OK; 
}

int 
zc_httpresp_cookie_toreq(zcHttpResp *resp, zcHttpReq *req)
{
    const char *key;
    zcCookie   *c;

    zc_dict_foreach_start(resp->cookie, key, c)
        zc_dict_add_str(req->cookie, key, zc_strdup(c->value.data, c->value.len));
    zc_dict_foreach_end

    return ZC_OK;
}

int
zc_httpresp_check_websocket(zcHttpResp *resp, zcHttpReq *req)
{
    char *magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    
    if (resp->code != 101) {
        ZCINFO("websocket return code error: %d", resp->code);
        return ZC_ERR;
    }

    char *connection = zc_dict_get_str(resp->header, "Connection", ""); 
    char *upgrade    = zc_dict_get_str(resp->header, "Upgrade", "");
    char *acceptkey  = zc_dict_get_str(resp->header, "Sec-WebSocket-Accept", NULL);

    if (strcasecmp(connection, "upgrade") != 0 || strcasecmp(upgrade, "websocket") != 0) {
        ZCINFO("websocket header connection/upgrade error:|%s|%s|", connection, upgrade);
        return ZC_ERR;
    }
   
    if (acceptkey == NULL) {
        ZCINFO("Sec-WebSocket-Accept error");
        return ZC_ERR;
    }

    char *reqkey = zc_dict_get_str(req->header, "Sec-WebSocket-Key", NULL);
    if (reqkey == NULL) {
        ZCINFO("Sec-WebSocket-Key not found in request");
        return ZC_ERR;
    }
    
    zcCString *buf = zc_cstr_alloc_stack(128);
    zc_cstr_init(buf, 128);
    ZCINFO("reqkey:%s, magic:%s", reqkey, magic);
    zc_cstr_append(buf, reqkey);
    zc_cstr_append(buf, magic);

    char encbuf[128] = {0};
    zc_sha1_base64(encbuf, buf->data, buf->len);

    if (strcmp(encbuf, acceptkey) != 0) {
        ZCINFO("accept key error, compute:%s, acceptkey:%s", encbuf, acceptkey);
        return ZC_ERR;
    }

    return ZC_OK;
}


