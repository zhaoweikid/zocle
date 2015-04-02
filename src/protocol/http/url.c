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
    if (strcmp(url->protocol.data, "https") == 0 || strcmp(url->protocol.data, "wss") == 0) {
        url->port = 443;
    }
    start = pos + 3; 
    pos = strchr(start, '/');
    char *portpos = strchr(start, ':');
    //ZCINFO("pos:%p portpos:%p", pos, portpos);
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

