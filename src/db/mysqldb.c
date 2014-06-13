#ifdef ZOCLE_WITH_MYSQL
#include <zocle/db/mysqldb.h>
#include <zocle/log/logfile.h>
#include <zocle/mem/alloc.h>
#include <string.h>

void
zc_mysqlcf_init(zcMySQLConf *cf, char *host, int port, char *user, char *pass, char *db, char *charset)
{
    memset(cf, 0, sizeof(zcMySQLConf));
    strncpy(cf->host, host, sizeof(cf->host));
    cf->port = port;
    strncpy(cf->user, user, sizeof(cf->user));
    strncpy(cf->pass, pass, sizeof(cf->pass));
    strncpy(cf->db, db, sizeof(cf->db));
    strncpy(cf->charset, charset, sizeof(cf->charset));
}
void
zc_mysqlcf_print(zcMySQLConf *cf)
{
    ZCINFO("host:\t%s\n", cf->host);
    ZCINFO("port:\t%d\n", cf->port);
    ZCINFO("user:\t%s\n", cf->user);
    ZCINFO("db:\t%s\n", cf->db);
    ZCINFO("charset:\t%s\n", cf->charset);
    ZCINFO("conn_timeout:\t%d\n", cf->conn_timeout);
    ZCINFO("read_timeout:\t%d\n", cf->read_timeout);
    ZCINFO("write_timeout:\t%d\n", cf->write_timeout);
}

zcDBRec* 
zc_mysqlrec_new(void *stmt)
{
    zcMySQLRec *mrec = (zcMySQLRec*)zc_malloc(sizeof(zcMySQLRec));
    memset(mrec, 0, sizeof(zcMySQLRec));

    mrec->res = stmt;

    mrec->_fields = 0;
    mrec->_rows = 0;
    mrec->_pos  = 0;
    mrec->row   = NULL;
    mrec->_get_row_next = 0;
    mrec->fieldmap = zc_dict_new(8, 0);

    if (stmt) {
        mrec->_fields = mysql_num_fields(mrec->res);
        mrec->_rows   = mysql_num_rows(mrec->res);
        mysql_data_seek(mrec->res, mrec->_pos);

        MYSQL_FIELD *fields;
        fields = mysql_fetch_fields(mrec->res);
        long i;
        for (i = 0; i < mrec->_fields; i++) {
            //mrec->fieldmap[fields[i].name] = i;
            zc_dict_add(mrec->fieldmap, fields[i].name, 0, (void*)i); 
        }
    }
    
    mrec->del             = zc_mysqlrec_delete;
    mrec->reset           = zc_mysqlrec_reset;
    mrec->row_next        = zc_mysqlrec_row_next;
    mrec->field_int_pos   = zc_mysqlrec_field_int_pos;
    mrec->field_int       = zc_mysqlrec_field_int;
    mrec->field_int64_pos = zc_mysqlrec_field_int64_pos;
    mrec->field_int64     = zc_mysqlrec_field_int64;
    mrec->field_float_pos = zc_mysqlrec_field_float_pos;
    mrec->field_float     = zc_mysqlrec_field_float;
    mrec->field_str_pos   = zc_mysqlrec_field_str_pos;
    mrec->field_str       = zc_mysqlrec_field_str;
    mrec->field_blob_pos  = zc_mysqlrec_field_blob_pos;
    mrec->field_blob      = zc_mysqlrec_field_blob;
    mrec->fetchone        = zc_mysqlrec_fetchone;

    return (zcDBRec*)mrec;
}
void 
zc_mysqlrec_delete(void *rec)
{
    zcMySQLRec *mrec = (zcMySQLRec*)rec;
    if (mrec->res) {
        mysql_free_result(mrec->res);
    } 
    zc_dict_delete(mrec->fieldmap);
    zc_free(mrec);
}

/*void 
zc_mysqlrec_close(zcDBRec *rec)
{
    if (mrec->res) {
        mysql_free_result(mrec->res);
    } 
    zc_dict_clear(mrec->fieldmap);
    mrec->res = NULL;
}

void 
zc_mysqlrec_clear(zcDBRec *rec)
{
}*/

int  
zc_mysqlrec_reset(zcDBRec *rec)
{
    zcMySQLRec *mrec = (zcMySQLRec*)rec;
    if (NULL == mrec->res)
        return -1;
    mrec->_pos = 0;
    mysql_data_seek(mrec->res, mrec->_pos);
    return 0;
}

int  
zc_mysqlrec_row_next(zcDBRec *rec)
{
    zcMySQLRec *mrec = (zcMySQLRec*)rec;
    if (NULL == mrec->res)
        return -1;
    mrec->_pos++;
    mrec->row = mysql_fetch_row(mrec->res);
    if (mrec->row == NULL)
        return -1;
    mrec->_get_row_next = 1;
    return 0;
}

int  
zc_mysqlrec_field_int_pos(zcDBRec *rec, int pos, int defv)
{
    zcMySQLRec *mrec = (zcMySQLRec*)rec;
    if (mrec->row == NULL || pos < 0 || pos >= mrec->_fields|| mrec->row[pos] == NULL)
        return defv;
    if (mrec->row[pos])
        return atoi(mrec->row[pos]);
    return defv;
}

int  
zc_mysqlrec_field_int(zcDBRec *rec, const char *name, int defv)
{
    zcMySQLRec *mrec = (zcMySQLRec*)rec;
    long pos = (long)zc_dict_get(mrec->fieldmap, name, 0, (void*)-1);
    return zc_mysqlrec_field_int_pos(rec, pos, defv);
}

int64_t 
zc_mysqlrec_field_int64_pos(zcDBRec *rec, int pos, int64_t defv)
{
    zcMySQLRec *mrec = (zcMySQLRec*)rec;
    if (mrec->row == NULL || pos < 0 || pos >= mrec->_fields || mrec->row[pos] == NULL)
        return defv;
    if (mrec->row[pos])
        return atoll(mrec->row[pos]);
    return defv; 
}

int64_t 
zc_mysqlrec_field_int64(zcDBRec *rec, const char *name, int64_t defv)
{
    zcMySQLRec *mrec = (zcMySQLRec*)rec;
    long pos = (long)zc_dict_get(mrec->fieldmap, name, 0, (void*)-1);
    return zc_mysqlrec_field_int64_pos(rec, pos, defv);
}

double  
zc_mysqlrec_field_float_pos(zcDBRec *rec, int pos, double defv)
{
    zcMySQLRec *mrec = (zcMySQLRec*)rec;
    if (mrec->row == NULL || pos < 0 || pos >= mrec->_fields || mrec->row[pos] == NULL)
        return defv;
    if (mrec->row[pos])
        return atof(mrec->row[pos]);
    return defv;
}

double  
zc_mysqlrec_field_float(zcDBRec *rec, const char *name, double defv)
{
    zcMySQLRec *mrec = (zcMySQLRec*)rec;
    long pos = (long)zc_dict_get(mrec->fieldmap, name, 0, (void*)-1);
    return zc_mysqlrec_field_float_pos(rec, pos, defv);
}

const char* 
zc_mysqlrec_field_str_pos(zcDBRec *rec, int pos, const char *defv)
{
    zcMySQLRec *mrec = (zcMySQLRec*)rec;
    if (mrec->row == NULL || pos < 0 || pos >= mrec->_fields || mrec->row[pos] == NULL)
        return (char*)defv;
    return mrec->row[pos]; 
}
const char* 
zc_mysqlrec_field_str(zcDBRec *rec, const char *name, const char *defv)
{
    zcMySQLRec *mrec = (zcMySQLRec*)rec;
    long pos = (long)zc_dict_get(mrec->fieldmap, name, 0, (void*)-1);
    return zc_mysqlrec_field_str_pos(rec, pos, defv);
}
const char* 
zc_mysqlrec_field_blob_pos(zcDBRec *rec, int pos, int *len, const char *defv)
{
    zcMySQLRec *mrec = (zcMySQLRec*)rec;
    if (mrec->row == NULL || pos < 0 || pos >= mrec->_fields || mrec->row[pos] == NULL)
        return (char*)defv;
    unsigned long *lengths = mysql_fetch_lengths(mrec->res);
    *len = lengths[pos];
    return mrec->row[pos]; 
}
const char* 
zc_mysqlrec_field_blob(zcDBRec *rec, const char *name, int *len, const char *defv)
{
    zcMySQLRec *mrec = (zcMySQLRec*)rec;
    long pos = (long)zc_dict_get(mrec->fieldmap, name, 0, (void*)-1);
    return zc_mysqlrec_field_blob_pos(rec, pos, len, defv);
}

int   
zc_mysqlrec_fetchone(zcDBRec *rec, const char *format, ...)
{
    return ZC_OK;
}
int   
zc_mysqlrec_fetchall(zcDBRec *rec, const char *format)
{
    return ZC_OK;
}
void* 
zc_mysqlrec_get_pos(zcDBRec *rec, int rowid, int pos, void *defv)
{
    return NULL;
}
void* 
zc_mysqlrec_get(zcDBRec *rec, int rowid, char *name, void *defv)
{
    return NULL;
}

// --- mysqldb

zcDB*    
zc_mysqldb_new(zcMySQLConf *cf)
{
    zcMySQLDB *mdb = (zcMySQLDB*)zc_malloc(sizeof(zcMySQLDB));
    memset(mdb, 0, sizeof(zcMySQLDB));
       
    memcpy(&mdb->conf, cf, sizeof(zcMySQLConf));
    mdb->_mysql = mysql_init(NULL);  
    if (NULL == mdb->_mysql) {
        ZCERROR("mysql init error!\n");
        zc_free(mdb);
        return NULL;
    }   
    
    if (cf->conn_timeout > 0) {
        mysql_options(mdb->_mysql, MYSQL_OPT_CONNECT_TIMEOUT, (const void*)&cf->conn_timeout);
    }
    if (cf->read_timeout > 0) {
        mysql_options(mdb->_mysql, MYSQL_OPT_READ_TIMEOUT, (const void*)&cf->read_timeout);
    }
    if (cf->write_timeout > 0) {
        mysql_options(mdb->_mysql, MYSQL_OPT_WRITE_TIMEOUT, (const void*)&cf->write_timeout);
    }
    
    mdb->_isopen = 0;

    mdb->del   = zc_mysqldb_delete;
    mdb->exec  = zc_mysqldb_exec;
    mdb->execf = zc_mysqldb_execf;
    mdb->query = zc_mysqldb_query;
    mdb->start = zc_mysqldb_start;
    mdb->commit   = zc_mysqldb_commit;
    mdb->rollback = zc_mysqldb_rollback;
    mdb->last_insert_id = zc_mysqldb_last_insert_id;

    zc_mysqldb_open((zcDB*)mdb);
    
    return (zcDB*)mdb;
}

void     
zc_mysqldb_delete(void *db)
{
    zcMySQLDB *mdb = (zcMySQLDB*)db;
    mysql_close(mdb->_mysql); 
    zc_free(mdb);
}

int 
zc_mysqldb_open(zcDB *db)
{
    zcMySQLDB *mdb = (zcMySQLDB*)db;
    ZCINFO("open host:%s:%d, db:%s, user:%s\n", 
            mdb->conf.host, mdb->conf.port, mdb->conf.db, mdb->conf.user);

    if (mysql_real_connect(mdb->_mysql, mdb->conf.host, mdb->conf.user, \
            mdb->conf.pass, mdb->conf.db, mdb->conf.port, NULL, 0) == NULL) {
        int err = mysql_errno(mdb->_mysql);
        ZCERROR("mysql connect error: %u:%s\n", err, mysql_error(mdb->_mysql));
        return -err; 
    }
    mysql_autocommit(mdb->_mysql, 1);
    zc_mysqldb_exec(db, "set names utf8");
    //zc_mysql_db_exec("set autocommit=1");
    mdb->_isopen = 1;
    return ZC_OK;
}

int
zc_mysqldb_close(zcDB *db)
{
    zcMySQLDB *mdb = (zcMySQLDB*)db;
    mysql_close(mdb->_mysql);
    mdb->_isopen = 0;
    mdb->_mysql = NULL;
    return 0;
}

int      
zc_mysqldb_exec(zcDB *db, const char *sql)
{
    zcMySQLDB *mdb = (zcMySQLDB*)db;
    int ret;
    /*if (!mdb->_isopen) {
        ret = zc_mysqldb_open(db);
        if (ret != ZC_OK) {
            return ret;
        }
    }*/
    int trycount = 3;
    while (1) {
        ret = mysql_real_query(mdb->_mysql, sql, strlen(sql));
        if (ret != 0) {
            int errcode = mysql_errno(mdb->_mysql);
            ZCWARN("execute error:%d %d %s, sql:%s", ret, errcode, mysql_error(mdb->_mysql), sql);
            if (errcode > 2000 && errcode <= 2006) {
                ZCWARN("reopen db, try again");
                zc_mysqldb_open(db);
                trycount--;

                if (trycount <= 0)
                    return -errcode;
                continue;
            }
            return -errcode;
        }
        break;
    }
    return 0;
}

int      
zc_mysqldb_execf(zcDB *db, const char *sql, const char *format, ...)
{
    return zc_mysqldb_exec(db, sql);
}

zcDBRec* 
zc_mysqldb_query(zcDB *db, const char *sql, const char *format, ...)
{
    zcMySQLDB *mdb = (zcMySQLDB*)db;
    
    int ret = zc_mysqldb_exec(db, sql);
    if (ret < 0) {
        if (mdb->err)
            *mdb->err = ret;
        return NULL;
    }
    MYSQL_RES *result = mysql_store_result(mdb->_mysql);
    return zc_mysqlrec_new(result);
}

int      
zc_mysqldb_start(zcDB *db)
{
    return zc_mysqldb_exec(db, "BEGIN");
}

int      
zc_mysqldb_commit(zcDB *db)
{
    zcMySQLDB *mdb = (zcMySQLDB*)db;
    return mysql_commit(mdb->_mysql);
}

int      
zc_mysqldb_rollback(zcDB *db)
{
    zcMySQLDB *mdb = (zcMySQLDB*)db;
    return mysql_rollback(mdb->_mysql);
}

int64_t
zc_mysqldb_last_insert_id(zcDB *db)
{
    zcMySQLDB *mdb = (zcMySQLDB*)db;
    return mysql_insert_id(mdb->_mysql);
}




#endif
