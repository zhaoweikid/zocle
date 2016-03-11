#include <zocle/protocol/redis.h>
#include <zocle/mem/alloc.h>
#include <zocle/ds/list.h>
#include <zocle/net/sockets.h>
#include <zocle/str/string.h>
#include <zocle/str/buffer.h>
#include <zocle/server/asynio.h>
#include <ev.h>

int
fast_atoi(const char *p)
{
    int x = 0;
    bool neg = false;
    if (*p == '-') {
        neg = true;
        ++p;
    }
    while (*p >= '0' && *p <= '9') {
        x = (x*10) + (*p - '0');
        ++p;
    }
    if (neg) {
        x = -x;
    }
    return x;
}

void
zc_redis_resp_print(zcRedisResp *r)
{
    ZCINFO("redis resp: type=%d int=%lld len=%d str=%s",r->type, r->integer, r->len, r->str );
    if(r->array != NULL){
        ZCINFO("list [");
        zcListNode *hnode;
        zc_list_foreach(r->array, hnode) {
            zc_redis_resp_print((zcRedisResp *)hnode->data);
        }
        ZCINFO("]");
    }
}

zcRedisResp*
zc_redis_resp_new()
{
    zcRedisResp *r = zc_calloct(zcRedisResp);
    r->type = ZC_REDIS_RESP_NIL;
    r->array = NULL;
    r->str = NULL;
    r->len = 0;

    return r;
}

void
zc_redis_resp_delete(void *x)
{
    zcRedisResp *r = (zcRedisResp *)x;
    if(r->len > 0 && r->str != NULL)
    {
        zc_free(r->str);
    }
    if(r->array != NULL)
    {
        zc_list_delete(r->array);
    }
    zc_free(r);
}

int
zc_redis_unpack(zcRedisResp *r,const char *data,const int dlen)
{
    int i = 1;

    //ZCINFO("data %s", data);
    switch(*data){
        case '+':
            r->type = ZC_REDIS_RESP_STATUS;

            while (*(data+i) != '\r') i++;
            r->len = i-1;
            r->str = zc_calloc(i);
            strncpy(r->str, data+1, i-1);
            i+=2;
            break;
        case '-':
            r->type = ZC_REDIS_RESP_ERROR;

            while (*(data+i) != '\r') i++;
            r->len = i-1;
            r->str = zc_calloc(i);
            strncpy(r->str, data+1, i-1);
            i+=2;
            break;
        case ':':
            r->type = ZC_REDIS_RESP_INTEGER;

            while(*(data+i) != '\r') i++;
            r->integer = fast_atoi(data+1);
            i+=2;
            break;
        case '$':
            r->type = ZC_REDIS_RESP_STRING;

            while(*(data+i) != '\r') i++;
            r->len = fast_atoi(data+1);

            // if len not -1
            if (r->len > 0)
            {
                r->str = zc_calloc(r->len+1);
                i += 2; //to string start
                strncpy(r->str, data+i, r->len);
                i = i + r->len + 2;
            }
            else
            {
                i += 2;
            }
            break;
        case '*':
            r->type = ZC_REDIS_RESP_ARRAY;

            while(*(data+i) != '\r') i++;
            r->len = fast_atoi(data+1);

            // if len not -1
            if(r->len > 0)
            {
                // init zclist
                r->array = zc_list_new();
                r->array->del = zc_redis_resp_delete;

                zcRedisResp* tmp;

                for(int j = 0;j < r->len;j++)
                {
                    tmp = zc_redis_resp_new();
                    int ret =  zc_redis_unpack(tmp, data+i+2, -1);
                    zc_list_append(r->array, (void *) tmp);
                    i+=ret;
                }
            }
            i+=2;
            break;
        ZCWARN("unknow first char %c", *data);
        return 0;
    }
    if(*(data + i - 2) == '\r' && *(data + i - 1) == '\n'){
        //如果完整解析解析
        /*ZCINFO("return %d",i);*/
        return i;
    }else{
        ZCWARN("buf not complete, need read more");
        return -1;
    }
}


int
zc_redis_execute(const char *host, const int port, const char *command, const int c_len, zcRedisResp *resp)
{
    zcSocket *sock = zc_socket_new_tcp(10);
    if (NULL == sock) {
        ZCERROR("tcp client create error!\n");
        zc_socket_delete(sock);
        return -1;
    }

    int ret = zc_socket_connect(sock, (char *)host, port);
    if (ret != ZC_OK) {
        ZCERROR("connect error! %d\n", ret);
        zc_socket_delete(sock);
        return -2;
    }

    ret = zc_socket_send(sock, (char *)command, c_len);
    if (ret != c_len)
    {
        ZCERROR("send error! %d\n", ret);
        return -3;
    }
    ret = zc_socket_send(sock, "\r\n", 2);
    if (ret != 2)
    {
        ZCERROR("send error! %d\n", ret);
        return -3;
    }

    zcString *s = zc_str_new(0);
    while(1)
    {
        char buf[4096] = {0};
        ret = zc_socket_recv(sock, buf, 4096);
        if (ret > 0)
        {
            buf[ret] = 0;
            zc_str_append(s, buf);
        }

        ret = zc_redis_unpack(resp, s->data, s->len);
        if (ret > 0){
             break;
        }
    }
    zc_socket_delete(sock);
    zc_str_delete(s);

    return ret;
}


#ifdef ZOCLE_WITH_LIBEV
int
__handler_read(zcAsynIO *conn)
{
    ZCINFO("redis read callback");
    const char *data = zc_buffer_data(conn->rbuf);
    /*ZCINFO("%s", data);*/
    int len = zc_buffer_used(conn->rbuf);
    zcRedisResp *r = zc_calloct(zcRedisResp);
    int ret = zc_redis_unpack(r, data, len);
    if (ret < 0){
         ZCWARN("unpack redis error");
    }else{
        ((int (*)(zcAsynIO*, zcRedisResp*)) conn->data)(conn, r);
        return ret;
    }
    return 0;
}

zcAsynIO*
zc_asynio_redis_new_client(const char *host, const int port, int timeout,
        struct ev_loop *loop, const char *command, int c_len, int (*callback)(zcAsynIO*, zcRedisResp*))
{
    zcAsynIO *conn = zc_asynio_new_tcp_client(host, port, timeout, NULL, loop, 2048, 2048);
    conn->data = callback;
    conn->p.handle_read = __handler_read;
    if(command != NULL)
    {
        zc_buffer_append(conn->wbuf, (void *)command, c_len);
        zc_buffer_append(conn->wbuf, "\r\n", 2);
    }
    zc_asynio_write_start(conn);
    return conn;
}

void
zc_asynio_redis_execute(zcAsynIO *conn, const char *command, const int c_len)
{
    zc_buffer_append(conn->wbuf, (void *)command, c_len);
    zc_buffer_append(conn->wbuf, "\r\n", 2);
}
#endif

