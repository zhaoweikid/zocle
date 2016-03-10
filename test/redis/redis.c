#include <zocle/zocle.h>
#include <ev.h>

void test1()
{

    zcRedisResp *r ;

    r = zc_redis_resp_new();
    zc_redis_unpack(r, "+OK\r\n" ,5);
    zc_redis_resp_print(r);
    zc_redis_resp_delete(r);

    r = zc_redis_resp_new();
    zc_redis_unpack(r, "-Error message\r\n" , 16);
    zc_redis_resp_print(r);
    zc_redis_resp_delete(r);

    r = zc_redis_resp_new();
    zc_redis_unpack(r, ":-2016\r\n" , 8);
    zc_redis_resp_print(r);
    zc_redis_resp_delete(r);

    r = zc_redis_resp_new();
    zc_redis_unpack(r, "$6\r\nfoobar\r\n" , 12);
    zc_redis_resp_print(r);
    zc_redis_resp_delete(r);

    r = zc_redis_resp_new();
    zc_redis_unpack(r, "*2\r\n$3\r\nfoo\r\n$3\r\nbar\r\n" , 22);
    zc_redis_resp_print(r);
    zc_redis_resp_delete(r);
}
void test2(){
    zcRedisResp *r ;
    r = zc_redis_resp_new();
    zc_redis_execute("172.100.101.151", 6379, "get 1", 5, r);
    zc_redis_resp_print(r);
    zc_redis_resp_delete(r);
}
int callback(zcAsynIO *conn, zcRedisResp *r){
    zc_redis_resp_print(r);
    zc_redis_resp_delete(r);

    /*zc_asynio_redis_execute(conn, "get 1", 5);*/
}

void test_asynio(){
    zcAsynIO *conn =  zc_asynio_redis_new_client("172.100.101.151", 6379, 3000, ev_default_loop(0),
            "get 1", 5, callback);
    ev_run(ev_default_loop(0), 0);

}
void test_1(){
    zcRedisResp *r ;

    r = zc_redis_resp_new();
    zc_redis_unpack(r, "*0\r\n" , 4);
    zc_redis_resp_print(r);
    zc_redis_resp_delete(r);

    r = zc_redis_resp_new();
    zc_redis_unpack(r, "*-1\r\n" , 5);
    zc_redis_resp_print(r);
    zc_redis_resp_delete(r);

}

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);
    zc_log_whole(_zc_log, 1);

    /*test1();*/
    /*test2();*/
    test_asynio();
    /*test_1();*/

}
