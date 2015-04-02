#include <zocle/protocol/http/cookie.h>
#include <ctype.h>
#include <zocle/mem/alloc.h>
#include <zocle/utils/datetime.h>

zcCookie* 
zc_cookie_new(const char *cookiestr)
{
    zcCookie *c = (zcCookie*)zc_calloc(sizeof(zcCookie));
    zc_str_init(&c->key, 0);
    zc_str_init(&c->value, 0);
    zc_str_init(&c->domain, 0);
    zc_str_init(&c->path, 0);

    if (zc_cookie_init(c, cookiestr) != ZC_OK) {
        zc_free(c);
        return NULL;
    }

    return c;
}

void	  
zc_cookie_delete(void *x)
{
    zcCookie *c = (zcCookie*)x;
    zc_str_destroy(&c->key);
    zc_str_destroy(&c->value);
    zc_str_destroy(&c->domain);
    zc_str_destroy(&c->path);

    zc_free(x);
}

int
zc_cookie_init(zcCookie *c, const char *cookiestr)
{
    if (cookiestr == NULL)
        return ZC_OK;
    
    //ZCINFO("cookie init");
    const char *s =  cookiestr;
    zcString *keybuf = zc_str_new(128);
    zcString *valbuf = zc_str_new(128);

    while (*s) {
        zc_str_clear(keybuf);
        zc_str_clear(valbuf);

        while (*s && isblank(*s)) s++;
        while (*s && *s != '=' && *s != ';') {
            zc_str_append_c(keybuf, *s);
            s++;
        }
        
        if (*s == ';') { // secure
            s++;
        }else{
            while (*s && (isblank(*s) || *s=='=')) s++;
            if (*s == '"') {
                s++; // skip "
                while (*s && *s != '"') {
                    zc_str_append_c(valbuf, *s);
                    s++;
                }
                if (*s == '"') s++;
            }else{
                while (*s && *s != ';' && *s != '\r' && *s != '\n') {
                    zc_str_append_c(valbuf, *s);
                    s++;
                }
            }
            while (*s && (*s == ';' || *s == '\r' || *s == '\n')) s++;
        }
        //ZCINFO("cookie key:%s value:%s", keybuf->data, valbuf->data);
        if (strcasecmp(keybuf->data, "expires") == 0) {
            zcDateTime dt;
            zc_datetime_init_format(&dt, "%a, %d-%b-%Y %H:%M:%S %z", valbuf->data);
            c->expires = dt.timestamp;
        }else if (strcasecmp(keybuf->data, "domain") == 0) {
            zc_str_assign(&c->domain, valbuf->data, valbuf->len);
        }else if (strcasecmp(keybuf->data, "path") == 0) {
            zc_str_assign(&c->path, valbuf->data, valbuf->len);
        }else if (strcasecmp(keybuf->data, "secure") == 0) {
            c->secure = true;
        }else if (strcasecmp(keybuf->data, "httponly") == 0) {
            c->httponly = true;
        }else{
            zc_str_assign(&c->key, keybuf->data, keybuf->len);
            zc_str_assign(&c->value, valbuf->data, valbuf->len);
        }
    }

    //ZCINFO("ok, parse cookie");
    zc_str_delete(keybuf);
    zc_str_delete(valbuf);
   
    return ZC_OK;
}


void	  
zc_cookie_print(zcCookie *c)
{
    ZCINFO("cookie: %s=%s, domain=%s, path=%s, expires=%u, secure=%d, httponly=%d", c->key.data, c->value.data, 
        c->domain.data, c->path.data, c->expires, c->secure, c->httponly);
}

