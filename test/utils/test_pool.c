#include <zocle/zocle.h>

typedef struct mytest
{
    int32_t id;
}MyTest;

int seq = 0;

void* mynew(void *x)
{
    seq++;
    MyTest *t = zc_calloct(MyTest);
    t->id = seq;
    ZCINFO("new %p\n", t);
    return t;
}


void mydel(void *x)
{
    ZCINFO("free %p\n", x);
    zc_free(x);
}

int test_pool()
{
    ZCINFO("====== test pool ======");
    zcPool  *pool = zc_pool_new(1, 10, 10, mynew, mydel);
    
    MyTest *t1 = zc_pool_get(pool, 0, NULL);
    sleep(1);
    MyTest *t2 = zc_pool_get(pool, 0, NULL);
    sleep(1);
    MyTest *t3 = zc_pool_get(pool, 0, NULL);
    sleep(1);
    zc_pool_put(pool, t1);
    zc_pool_put(pool, t2);
    zc_pool_put(pool, t3);


    int n = 1000;
    int i;
    for (i=0; i<n; i++) {
        MyTest *t = zc_pool_get(pool, 0, NULL);
        if (NULL == t) {
            ZCERROR("pool get error!");
            return -1;
        }
        ZCINFO("%d get %d\n", i, t->id);
        zc_pool_put(pool, t);
        usleep(10000);
    }

    zc_pool_delete(pool);

    return 0;
}

// test db connection
int test_db()
{
    ZCINFO("====== test db ======");
    zcMySQLConf dbconf;
   
    zc_mysqlcf_init(&dbconf, "127.0.0.1", 3306, "root", "123456", "test", "utf8");


    zcPool  *pool = zc_pool_new(1, 10, 60, (zcFuncNew)zc_mysqldb_new, zc_mysqldb_delete);
    pool->userdata = &dbconf; 

    
    zcDB *t1 = zc_pool_get(pool, 0, NULL);
    sleep(1);
    zcDB *t2 = zc_pool_get(pool, 0, NULL);
    sleep(1);
    zcDB *t3 = zc_pool_get(pool, 0, NULL);
    sleep(1);
    zc_pool_put(pool, t1);
    zc_pool_put(pool, t2);
    zc_pool_put(pool, t3);


    int n = 1000;
    int i;
    for (i=0; i<n; i++) {
        zcDB *db = zc_pool_get(pool, 0, NULL);
        if (NULL == db) {
            ZCERROR("pool get error!");
            return -1;
        }
        //ZCINFO("%d get %p, open:%d\n", i, db, ((zcMySQLDB*)db)->_isopen);
        ZCINFO("%d get %p\n", i, db);
        zcDBRec *rec = db->query(db, "select version()", NULL);
        if (NULL == rec) {
            ZCWARN("query error!");
        }else{
            rec->row_next(rec);
            ZCINFO("query: %s, num:%lld", rec->field_str_pos(rec, 0, ""), ((zcMySQLRec*)rec)->_rows);
        }

        zc_pool_put(pool, db);
        usleep(10000);
    }

    zc_pool_delete(pool);

    return 0;

}




int main()
{
    zc_log_new("stdout", ZC_LOG_ALL);

    //test_pool();
    test_db();

    return 0;
}

