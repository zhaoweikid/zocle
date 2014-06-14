#include <zocle/internet/http/httpreq.h>
#include <zocle/mem/alloc.h>
#include <zocle/utils/funcs.h>
#include <zocle/internet/http/httpheader.h>
#include <zocle/enc/url.h>
#include <zocle/str/string.h>
#include <zocle/str/cstrlist.h>
#ifndef _WIN32
#include <netdb.h>
#include <arpa/inet.h>
#endif

#include <ctype.h>

zcURL* 
zc_url_new(const char *urlstr)
{
    zcURL *url = zc_malloc(sizeof(zcURL));
    if (zc_url_init(url, urlstr) != ZC_OK) {
        zc_url_delete(url);
        return NULL;
    }
    return url;
}

int
zc_url_init(zcURL *url, const char *urlstr)
{
    zc_str_init(&url->url, 32);
    zc_str_init(&url->protocol, 8);
    zc_str_init(&url->username, 16);
    zc_str_init(&url->password, 16);
    zc_str_init(&url->domain, 24);
    zc_str_init(&url->path, 32);
    zc_str_init(&url->params, 8);
    zc_str_init(&url->query, 8);
    url->port = 80;

    if (urlstr) {
        if (zc_url_parse(url, urlstr) != ZC_OK) {
            ZCWARN("url parse error:%s", urlstr);
            zc_url_clear(url);
            return ZC_ERR;
        }
    }

    return ZC_OK;
}
 
void   
zc_url_delete(void *x)
{
    zc_url_destroy(x);
    zc_free(x);
}

void   
zc_url_destroy(void *x)
{
    zcURL *url = (zcURL*)x;

    zc_str_destroy(&url->url);
    zc_str_destroy(&url->protocol);
    zc_str_destroy(&url->username);
    zc_str_destroy(&url->password);
    zc_str_destroy(&url->domain);
    zc_str_destroy(&url->path);
    zc_str_destroy(&url->params);
    zc_str_destroy(&url->query);
}
int    
zc_url_parse(zcURL *url, const char *urlstr)
{
    zc_str_assign(&url->url, urlstr, 0);

    // http://user:pass@www.xxx.com:port/p1/p2/p3.py?a=100&b=2;
    const char *start = urlstr; 
    char *pos = strstr(start, "://");
    if (NULL == pos)
        return ZC_ERR;
   
    // find protocol
    zc_str_assign(&url->protocol, urlstr, pos-urlstr);
    start = pos + 3; 
    pos = strchr(start, '/');
    char *portpos = strchr(start, ':');
    ZCINFO("pos:%p portpos:%p", pos, portpos);
    if (portpos && (pos == NULL || portpos < pos)) {
        char portbuf[8] = {0};
        if (pos) {
            strncpy(portbuf, portpos+1, pos-portpos-1);
        }else{
            strcpy(portbuf, portpos+1);
        }
        url->port = atoi(portbuf);
    }

    if (NULL == pos){
        //return ZC_ERR;
        if (portpos) {
            zc_str_assign(&url->domain, start, portpos-start);
        }else{
            zc_str_assign(&url->domain, start, 0);
        }
        zc_str_assign(&url->path, "/", 0);
        return ZC_OK;
    }

    char *upos = strchr(start, '@');
    if (upos != NULL && upos < pos) {
        char *upos1 = strchr(start, ':'); 
        if (NULL == upos1) {
            return ZC_ERR;
        }
        zc_str_assign(&url->username, start, upos1-start);
        zc_str_assign(&url->password, upos1+1, upos-upos1-1);
        start = upos+1;
    }
    if (portpos && portpos < pos) {
        zc_str_assign(&url->domain, start, portpos-start);
    }else{
        zc_str_assign(&url->domain, start, pos-start);
    }
    start = pos;

    pos = strchr(start, ';');
    if (NULL != pos) {
        zc_str_assign(&url->path, start, pos-start); 
        zc_str_assign(&url->params, pos+1, 0);
        return ZC_OK;
    }

    pos = strchr(start, '?');
    if (NULL != pos) {
        zc_str_assign(&url->path, start, pos-start); 
        zc_str_assign(&url->query, pos+1, 0);
        return ZC_OK;
    }
    if (*start) {
        zc_str_assign(&url->path, start, 0);
    }else{
        zc_str_assign(&url->path, "/", 0);
    }
    return ZC_OK;
}

bool   
zc_url_isip(zcURL *url)
{
    const char *start = strstr(url->url.data, "://");
    if (NULL == start) {
        start = url->url.data;
    }else{
        start += 3;
    }
    ZCINFO("start:%s", start);
    int  i, n;
    for (n=0; n<4; n++) {
        i = 0;
        while (*start && isdigit(*start)) {
            if (i >= 3)
                return false;
            start++;
            i++;
        }
        ZCINFO("i:%d n:%d start:%c", i, n, *start);
        if (i == 0) return false;
        if (n < 3 && *start != '.') return false;
        if (n == 3 && *start != 0 && *start != ':' && *start != '/') return false;
        start++;
    }
    return true;
}

bool   
zc_host_isip(const char *domain)
{
    const char *start = domain;
    int  i, n;
    for (n=0; n<4; n++) {
        i = 0;
        while (*start && isdigit(*start)) {
            if (i >= 3)
                return false;
            start++;
            i++;
        }
        if (i == 0) return false;
        if (n < 3 && *start != '.') return false;
        if (n == 3 && *start != 0) return false;
        start++;
    }
    return true;
}

bool   
zc_url_domain_isip(zcURL *url)
{
    const char *domain = url->domain.data;
    return zc_host_isip(domain);
}

void   
zc_url_clear(zcURL *url)
{
    zc_str_clear(&url->url);
    zc_str_clear(&url->protocol);
    zc_str_clear(&url->username);
    zc_str_clear(&url->password);
    zc_str_clear(&url->domain);
    zc_str_clear(&url->path);
    zc_str_clear(&url->params);
    zc_str_clear(&url->query);
}

void   
zc_url_print(zcURL *url)
{
    ZCINFO("url: %s", url->url.data); 
    ZCINFO("port: %d", url->port);
    ZCINFO("protocol: %s", url->protocol.data); 
    ZCINFO("username: %s", url->username.data); 
    ZCINFO("password: %s", url->password.data); 
    ZCINFO("domain: %s", url->domain.data); 
    ZCINFO("path: %s", url->path.data); 
    ZCINFO("params: %s", url->params.data); 
    ZCINFO("query: %s", url->query.data); 
}

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


