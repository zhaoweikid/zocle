#ifdef ZOCLE_WITH_MYSQL
#include <zocle/zocle.h>
#include <assert.h>

int test1()
{
    zcMySQLConf conf;

    zc_mysqlcf_init(&conf, "127.0.0.1", 3306, "root", "", "test", "utf8");
    zc_mysqlcf_print(&conf);

    zcDB *db = zc_mysqldb_new(&conf);
    assert(db != NULL);


    zcDBRec *rec = zc_mysqldb_query(db, "select now();", NULL);
    assert(rec != NULL);

    zc_mysqlrec_row_next(rec);
    ZCINFO("query:%s\n", zc_mysqlrec_field_str_pos(rec, 0, ""));

    zc_mysqldb_delete(db);

    return 0;
}

int test2()
{
    zcMySQLConf conf;

    zc_mysqlcf_init(&conf, "127.0.0.1", 3306, "root", "", "test", "utf8");
    zc_mysqlcf_print(&conf);

    zcDB *db = zc_mysqldb_new(&conf);
    assert(db != NULL);


    zcDBRec *rec = db->query(db, "select now();", NULL);
    assert(rec != NULL);

    rec->row_next(rec);
    ZCINFO("query:%s\n", rec->field_str_pos(rec, 0, ""));
    rec->del(rec);

    db->del(db);

    return 0;
}



int main()
{
    zc_log_new("stdout", ZC_LOG_ALL);
    ZCINFO("====== test1 ======\n");
    test1();
    ZCINFO("====== test2 ======\n");
    test2();
}

#else
int main() { return 0; }
#endif
