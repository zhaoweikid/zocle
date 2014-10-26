#include <zocle/net/socketio.h>
#include <zocle/mem/alloc.h>
#include <zocle/log/logfile.h>

zcSocketIO* 
zc_sockio_new (zcSocket *s, int size)
{
    zcSocketIO  *sf;
    
    sf = (zcSocketIO*)zc_malloc(sizeof(zcSocketIO));
    memset(sf, 0, sizeof(zcSocketIO));
    sf->rbuf = zc_buffer_new(size);
    sf->sock = s;
    return sf;
}

void
zc_sockio_delete (void *s)
{
    zcSocketIO  *sf = s;
    zc_buffer_delete(sf->rbuf);
    //zc_socket_delete(sf->sock);
    zc_free(sf);
}

void
zc_sockio_delete4_ (void *s, void *obj, const char *file, int line)
{
    zc_sockio_delete(s);
}


int
zc_sockio_read (zcSocketIO *s, char *buf, int len)
{
    int ret;

    /*if (len > s->rbuf->size) {
        int newsize = (len/4096 + ((len%4096==0)?1:0)) * 4096;
        s->rbuf = zc_buffer_resize(s->rbuf, newsize);
    }*/
    
    if (zc_buffer_used(s->rbuf) == 0) {
        zc_buffer_reset(s->rbuf);
        ret = zc_socket_recv(s->sock, s->rbuf->data, s->rbuf->size);
        if (ret <= 0) {
            return ret;
        }else{
            s->rbuf->end = ret;
        }
    }
    return zc_buffer_get(s->rbuf, buf, len); 
}

int zc_sockio_read_nocopy (zcSocketIO *s, int len)
{
    int ret;

    if (len > s->rbuf->size) {
        int newsize = (len/4096 + ((len%4096>0)?1:0)) * 4096;
        s->rbuf = zc_buffer_resize(s->rbuf, newsize);
    }else{
        zc_buffer_compact(s->rbuf);
    }
    zcBuffer *rbuf = s->rbuf;
    //ZCINFO("read nocopy len:%d\n", rbuf->end);
    while (zc_buffer_used(rbuf) < len) {
        ret = zc_socket_recv(s->sock, rbuf->data+rbuf->end, len-rbuf->end);
        if (ret < 0) {
            return ret;
        }else if (ret == 0) {
            break;
        }else{
            s->rbuf->end += ret;
        }
    }
    return zc_buffer_used(s->rbuf); 
}

int
zc_sockio_readn (zcSocketIO *s, char *buf, int len)
{
    size_t      nleft = len;
    ssize_t     nread;
    char        *bufp = buf;
    
    while (nleft > 0) {
        nread = zc_sockio_read(s, bufp, nleft);
        if (nread < 0) {
            if (nleft == len)
                return ZC_ERRNO; 
            else
                break;
        }else if (nread == 0) {
            break;
        }
        nleft -= nread;
        bufp += nread;
    }

    return len - nleft;
}

int
zc_sockio_readline (zcSocketIO *s, char *buf, int len)
{
    int     n, rc;
    char    c = 0, *bufp = buf;
    
    len--; // ignore \0
    for (n = 0; n < len; n++) {
        if ((rc = zc_sockio_read (s, &c, 1)) == 1) {
            *bufp++ = c;
            if (c == '\n') {
                n++;
                break;
            }
        }else if (rc == 0){
            break;
        }else{
            return -1;
        }
    }
    *bufp = 0;

    return n;
}

zcString* 
zc_sockio_readline_str (zcSocketIO *s, zcString *str)
{
    int     rc;
    char    c;
   
    if (NULL == str) {
        str = zc_str_new(8192);
    }
    while (1) {
        if ((rc = zc_sockio_read (s, &c, 1)) == 1) {
            zc_str_append_c(str, c);
            if (c == '\n')
                break;
        }else if (rc == 0){
            break;
        }else{
            ZCWARN("zc_sockio_read return %d, %s\n", rc, strerror(errno));
            break;
        }
    }
    return str;
}

int 
zc_sockio_readline_cstr (zcSocketIO *s, zcCString *str)
{
    int     rc;
    char    c;
   
    while (1) {
        if ((rc = zc_sockio_read (s, &c, 1)) == 1) {
            if (zc_cstr_append_c(str, c) < 0) {
                return ZC_ERR;
            }
            if (c == '\n')
                break;
        }else if (rc == 0){
            break;
        }else{
            ZCWARN("zc_sockio_read return %d, %s\n", rc, strerror(errno));
            break;
        }
    }
    return ZC_OK;
}

zcList* 
zc_sockio_readlines (zcSocketIO *s, int num)
{
    zcList   *list = NULL;
    zcString *str;
    int     i = 0;

    while(1) {
        str = zc_sockio_readline_str(s, NULL);
        if (NULL == str)
            break;
        if (str->len == 0) {
            zc_str_delete(str);
            break;
        }
        
        if (NULL == list) {
            list = zc_list_new();
            list->del = zc_str_delete;
            if (NULL == list) {
                ZCERROR("zlist_new error!");
            }
        }
        zc_list_append(list, str);
        i++;

        if (num > 0 && i == num)
            break;
    }

    return list;
}

zcList* 
zc_sockio_readlines_endline (zcSocketIO *s, char *endline)
{
    zcList   *list = NULL;
    zcString *str;

    while(1) {
        str = zc_sockio_readline_str(s, NULL);
        if (NULL == str)
            break;
        if (str->len == 0) {
            zc_str_delete(str);
            break;
        }
        
        if (NULL == list) {
            list = zc_list_new();
            list->del = zc_str_delete;
            if (NULL == list) {
                ZCERROR("zlist_new error!");
            }
        }
        zc_list_append(list, str);
        
        if (strcmp(str->data, endline) == 0)
            break;
    }

    return list;
}

zcList* 
zc_sockio_readlines_custom (zcSocketIO *s, zcLineTerminatorFunc linecheck)
{
    zcList   *list = NULL;
    zcString *str;

    while(1) {
        str = zc_sockio_readline_str(s, NULL);
        if (NULL == str)
            break;
        if (str->len == 0) {
            zc_str_delete(str);
            break;
        }
        
        if (NULL == list) {
            list = zc_list_new();
            list->del = zc_str_delete;
            if (NULL == list) {
                ZCERROR("zlist_new error!");
            }
        }
        zc_list_append(list, str);

        if (linecheck(str->data, str->len)) {
            break;
        }
    }

    return list;
}


int
zc_sockio_write (zcSocketIO *s, char *buf, int len)
{
    return zc_socket_send(s->sock, buf, len);
}

int
zc_sockio_writen (zcSocketIO *s, char *buf, int len)
{
    return zc_socket_sendn(s->sock, buf, len);
}

int
zc_sockio_write_list (zcSocketIO *s, zcList *list)
{
    zcString *str;
    int     ret;
    int     count = 0;
    
    zcListNode *node;
    zc_list_foreach(list, node) {
        str = (zcString*)node->data;
        ret = zc_socket_sendn(s->sock, str->data, str->len);
        if (ret != str->len) {
            return ret;
        }
        count++;
    }
    return count;
}


