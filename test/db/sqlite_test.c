#include <zocle/zocle.h>
#include <assert.h>

#ifdef ZOCLE_WITH_SQLITE
void test1()
{
    zcDB *db = zc_sqlitedb_new("test.db");
    assert(db != NULL);
   
    zc_sqlitedb_exec(db, "create table if not exists test(id integer primary key autoincrement, name varchar(128))");

    zcCString buf[ZC_CSTR_SIZE(1024)];
    zc_cstr_init(buf, 1024);

    int ret;
    int i = 0;
    for (i=0; i<100; i++) {
        zc_cstr_format(buf, "insert into test(name) values ('n%d');", i);
        ZCINFO("sql:%s\n", buf->data);
        ret = zc_sqlitedb_exec(db, buf->data); 
        assert(ret == ZC_OK);
    }

    for (i=100; i<200; i++) {
        //zc_cstr_format(buf, "insert into test(name) values ('n%d');", i);
        zc_cstr_format(buf, "x%d", i);
        //ZCINFO("sql:%s\n", buf->data);
        ret = zc_sqlitedb_execf(db, "insert into test(name) values (?)", "s", buf->data); 
        ZCINFO("i:%d ret:%d\n", i, ret);
        assert(ret == ZC_OK);
    }
    
    ZCINFO("1 query ...\n");
    zcDBRec *rec = zc_sqlitedb_query(db, "select id,name from test", NULL);
    assert(rec != NULL);
    
    while (zc_sqliterec_row_next(rec) == ZC_OK) {
        ZCINFO("row id:%d name:%s\n", zc_sqliterec_field_int_pos(rec,0,0), zc_sqliterec_field_str(rec,"name",""));  
    }
    zc_sqliterec_delete(rec);
    
    ZCINFO("2 query ...\n");
    rec = zc_sqlitedb_query(db, "select id,name from test", NULL);
    assert(rec != NULL);
    
    int  id;
    char name[128] = {0};
    while (zc_sqliterec_row_next(rec) == ZC_OK) {
        zc_sqliterec_fetchone(rec, "is", &id, name);
        ZCINFO("row id:%d name:%s\n", id, name);  
    }
    zc_sqliterec_delete(rec);

    zc_sqlitedb_delete(db);
}

void test2()
{
    zcDB *db = zc_sqlitedb_new("test.db");
    assert(db != NULL);
   
    db->exec(db, "create table if not exists test(id integer primary key autoincrement, name varchar(128))");

    zcCString buf[ZC_CSTR_SIZE(1024)];
    zc_cstr_init(buf, 1024);

    int ret;
    int i = 0;
    for (i=0; i<100; i++) {
        zc_cstr_format(buf, "insert into test(name) values ('n%d');", i);
        ZCINFO("sql:%s\n", buf->data);
        ret = db->exec(db, buf->data); 
        assert(ret == ZC_OK);
    }

    for (i=100; i<200; i++) {
        //zc_cstr_format(buf, "insert into test(name) values ('n%d');", i);
        zc_cstr_format(buf, "x%d", i);
        //ZCINFO("sql:%s\n", buf->data);
        ret = db->execf(db, "insert into test(name) values (?)", "s", buf->data); 
        ZCINFO("i:%d ret:%d\n", i, ret);
        assert(ret == ZC_OK);
    }
    
    ZCINFO("1 query ...\n");
    zcDBRec *rec = db->query(db, "select id,name from test", NULL);
    assert(rec != NULL);
    
    while (rec->row_next(rec) == ZC_OK) {
        ZCINFO("row id:%d name:%s\n", rec->field_int_pos(rec,0,0), rec->field_str(rec,"name",""));  
    }
    rec->del(rec);
    
    ZCINFO("2 query ...\n");
    rec = db->query(db, "select id,name from test", NULL);
    assert(rec != NULL);
    
    int  id;
    char name[128] = {0};
    while (rec->row_next(rec) == ZC_OK) {
        rec->fetchone(rec, "is", &id, name);
        ZCINFO("row id:%d name:%s\n", id, name);  
    }
    rec->del(rec);

    db->del(db);
}
#endif

int main()
{
    zc_log_new("stdout", ZC_LOG_ALL);

#ifdef ZOCLE_WITH_SQLITE
    ZCINFO("====== test1 ======\n");
    test1();
    ZCINFO("====== test2 ======\n");
    test2();
#endif   

    return 0;
}


