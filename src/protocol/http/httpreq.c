#include <zocle/protocol/http/httpreq.h>
#include <zocle/protocol/http/url.h>
#include <zocle/mem/alloc.h>
#include <zocle/utils/funcs.h>
#include <zocle/protocol/http/httpheader.h>
#include <zocle/enc/url.h>
#include <zocle/str/string.h>
#ifndef _WIN32
#include <netdb.h>
#include <arpa/inet.h>
#endif

#include <ctype.h>


zcHttpReq* 
zc_httpreq_new(const char *urlstr)
{
    if (NULL == urlstr)
        return NULL;
    zcHttpReq *req = zc_malloc(sizeof(zcHttpReq));
    memset(req, 0, sizeof(zcHttpReq));
    
    req->method = ZC_HTTP_GET;
    req->protocol = ZC_HTTP_P11;
    //zc_str_init(&req->uri, 128);
    //zc_str_assign(&req->uri, "/", 1);
    zc_url_init(&req->url, urlstr);
    req->header = zc_dict_new_full(512, 0, zc_free_func, zc_free_func);
    zc_dict_add(req->header, "User-Agent", 0, zc_strdup("Zocle/1.0",0));
    zc_dict_add(req->header, "Accept", 0, zc_strdup("*/*",0));
    zc_dict_add(req->header, "Connection", 0, zc_strdup("Keep-Alive",0));
    zc_dict_add(req->header, "Accept-Encoding", 0, zc_strdup("gzip",0));
    zc_str_init(&req->body, 512);
   
    char *p = req->url.protocol.data;

    // websocket
    if (strcmp(p, "ws") == 0 || strcmp(p, "wss") == 0) {
        zc_httpreq_header_websocket(req);
        req->websocket = 1;
    }

    return req;
}

void
zc_httpreq_delete(void *x)
{
    zcHttpReq *req = (zcHttpReq*)x;

    zc_dict_delete(req->header);
    zc_str_destroy(&req->body);
    //zc_str_destroy(&req->uri);
    zc_url_destroy(&req->url);
    zc_free(x);
}

/*int	
zc_httpreq_send(zcHttpReq *req, zcHttpConn *conn)
{
    zcString s;

    zc_str_init(&s, 1024);
    zc_httpreq_header_str(req, &s);
    
    int ret = zc_socket_sendn(conn->sock, s.data, s.len);
    if (ret != s.len) {
        zc_str_destroy(&s);
        return ret;
    }
    zc_str_destroy(&s);
    ret = zc_socket_sendn(conn->sock, req->body.data, req->body.len);
    if (ret != req->body.len) {
        return ret;
    }
    return ZC_OK;
}*/

int
zc_httpreq_header_tostr(zcHttpReq *req, zcString *s)
{
    char *key, *value;
    char *methods[]   = {"GET", "POST", "HEAD", "PUT", "DELETE", "TRACE", "CONNECT", "OPTIONS", NULL};
    char *protocols[] = {"HTTP/1.0", "HTTP/1.1", NULL};

    zc_str_append_format(s, "%s %s %s\r\n", methods[req->method], req->url.path.data, protocols[req->protocol]);
    if (req->header->len > 0) {
        zc_dict_foreach_start(req->header, key, value)
            zc_str_append_format(s, "%s: %s\r\n", key, value);
        zc_dict_foreach_end
    }
    if (!zc_dict_haskey(req->header, "Host", 0)) {
        zc_str_append_format(s, "Host: %s\r\n", req->url.domain.data);
    }
    
    if (req->cookie && req->cookie->len > 0) {
        zcString cookiestr;
        zc_str_init(&cookiestr, 128);

        zc_dict_foreach_start(req->header, key, value)
            zc_str_append_format(s, "%s=%s;", key, value);
        zc_dict_foreach_end

        zc_str_rtrim(s, ';');
        zc_str_append_format(s, "Cookie: %s\r\n", cookiestr.data);
        zc_str_destroy(&cookiestr);
    }
    if (req->body.len > 0) {
        zc_str_append_format(s, "Content-Length: %d\r\n", req->body.len);
    }
    //zc_str_append("\r\n");
    return ZC_OK;
}


int	
zc_httpreq_form(zcHttpReq *req, zcDict *form)
{
    if (form->len <= 0)
        return ZC_ERR;
    char *key, *value;

    zcString *s = zc_str_new(128);
    zc_dict_foreach_start(req->header, key, value)
        zc_str_append_format(s, "%s=%s&", key, value);
    zc_dict_foreach_end
    zc_str_rtrim(s, '&');
   
    int outlen = s->len*3+1;
    char *out = (char*)zc_malloc(outlen);
    //memset(out, 0, outlen); 

    int ret = zc_url_enc(out, s->data, s->len);
    out[ret] = 0;
    
    zc_str_assign(&req->body, out, ret); 

    zc_free(out);
    zc_str_delete(s);

    return ZC_OK;
}

int	
zc_httpreq_header_websocket(zcHttpReq *req)
{
    req->method = ZC_HTTP_GET;

    zc_dict_set_str(req->header, "Connection", zc_strdup("Upgrade",0));
    zc_dict_set_str(req->header, "Upgrade", zc_strdup("WebSocket",0));
    zc_dict_set_str(req->header, "Sec-WebSocket-Origin", zc_strdup("no",0));
    zc_dict_set_str(req->header, "Sec-WebSocket-Key", zc_strdup("4tAjitqO9So2Wu8lkrsq3w==",0));
    zc_dict_set_str(req->header, "Sec-WebSocket-Version", zc_strdup("8",0));
    
    return ZC_OK;
}


