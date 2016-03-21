#include <stdio.h>
#include <stdint.h>

#include <zocle/base/defines.h>
#include <zocle/ds/list.h>
#include <zocle/str/string.h>
#include <zocle/server/asynio.h>

enum zcRedisRespType {
    ZC_REDIS_RESP_STRING  = 1,
    ZC_REDIS_RESP_ARRAY   = 2,
    ZC_REDIS_RESP_INTEGER = 3,
    ZC_REDIS_RESP_NIL     = 4,
    ZC_REDIS_RESP_STATUS  = 5,
    ZC_REDIS_RESP_ERROR   = 6
};

typedef struct
{
    int       type;     // resp type
    long long integer;  // if resp is integer
    int       len;      // len of string
    char      *str;     // string
    zcList    *array;   // list of resp
    void      *data;    // customer data
}zcRedisResp;

void         zc_redis_resp_print(zcRedisResp *);

zcRedisResp* zc_redis_resp_new();
void         zc_redis_resp_delete(void *);

int zc_redis_unpack(zcRedisResp *r, const char *data, const int dlen);

int zc_redis_execute(const char *host, const int port, const char *command, const int c_len, zcRedisResp *resp);

#ifdef ZOCLE_WITH_LIBEV
zcAsynIO* zc_asynio_redis_new_client(const char *host, const int port, int timeout,
        struct ev_loop *loop, const char *command, const int c_len, int (*callback)(zcAsynIO*, zcRedisResp*));
void zc_asynio_redis_execute(zcAsynIO *conn, const char *command, const int c_len);
#endif



