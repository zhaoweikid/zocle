#ifdef ZOCLE_WITH_SQLITE
#include <zocle/db/sqlitedb.h>
#include <zocle/base/defines.h>
#include <zocle/mem/alloc.h>
#include <string.h>
#include <zocle/log/logfile.h>

zcDBRec* 
zc_sqliterec_new(void *stmt)
{
    zcSQLiteRec *rec = (zcSQLiteRec*)zc_malloc(sizeof(zcSQLiteRec));
    memset(rec, 0, sizeof(rec));

    rec->stmt = stmt;
    rec->fields = rec->rows = 0;
    memset(rec->field_types, 0, sizeof(rec->field_types));

    rec->field_names = zc_dict_new(8, 0);
    //rec->field_names->valdel = NULL;
    rec->data = NULL;

    rec->del             = zc_sqliterec_delete;
    rec->reset           = zc_sqliterec_reset;
    rec->row_next        = zc_sqliterec_row_next;
    rec->field_int_pos   = zc_sqliterec_field_int_pos;
    rec->field_int       = zc_sqliterec_field_int;
    rec->field_int64_pos = zc_sqliterec_field_int64_pos;
    rec->field_int64     = zc_sqliterec_field_int64;
    rec->field_float_pos = zc_sqliterec_field_float_pos;
    rec->field_float     = zc_sqliterec_field_float;
    rec->field_str_pos   = zc_sqliterec_field_str_pos;
    rec->field_str       = zc_sqliterec_field_str;
    rec->field_blob_pos  = zc_sqliterec_field_blob_pos;
    rec->field_blob      = zc_sqliterec_field_blob;
    rec->fetchone        = zc_sqliterec_fetchone;

    return (zcDBRec*)rec;
}

void 
zc_sqliterec_delete(void *rec)
{
    zcSQLiteRec *srec = (zcSQLiteRec*)rec;
    if (srec->stmt) {
        sqlite3_finalize(srec->stmt);
    }
    if (srec->field_names) {
        zc_dict_delete(srec->field_names);
    }
    if (srec->data) {
        zc_array_delete(srec->data);
    }

    zc_free(rec);
}

/*void 
zc_sqliterec_close(zcDBRec *rec)
{
    zcSQLiteRec *srec = (zcSQLiteRec*)rec;
    if (srec->stmt) {
        sqlite3_finalize(srec->stmt);
    }
    srec->stmt = NULL;
}

void 
zc_sqliterec_clear(zcDBRec *rec)
{
    zcSQLiteRec *srec = (zcSQLiteRec*)rec;
    srec->fields = srec->rows = 0;
    memset(srec->field_types, 0, sizeof(srec->field_types));
    if (srec->stmt) {
        sqlite3_finalize(srec->stmt);
    }
    srec->stmt = NULL;
    zc_dict_clear(srec->field_names);
    if (srec->data) {
        zc_array_delete(srec->data);
        srec->data = NULL;
    }
}*/

int  
zc_sqliterec_reset(zcDBRec *rec)
{
    zcSQLiteRec *srec = (zcSQLiteRec*)rec;
    if (NULL == srec->stmt)
        return ZC_ERR;
    int ret = sqlite3_reset(srec->stmt);
    if (ret != 0)
        return -ret;
    return ZC_OK;
}

int  
zc_sqliterec_row_next(zcDBRec *rec)
{
    zcSQLiteRec *srec = (zcSQLiteRec*)rec;
    if (NULL == srec->stmt) {
        ZCWARN("record stmt is null");
        return ZC_ERR;
    }
    int ret = sqlite3_step(srec->stmt);
    if (ret == SQLITE_ROW) {
        srec->rows++;
        if (srec->fields == 0) {
            srec->fields = sqlite3_column_count(srec->stmt);
            if (srec->fields > (int)(sizeof(srec->field_types)/sizeof(int))) {
                ZCERROR("fields too long.\n");
                return ZC_ERR;
            }
            long i;
            for (i = 0; i < srec->fields; i++) {
                int coltype = sqlite3_column_type(srec->stmt, i);
                srec->field_types[i] = coltype;
                //srec->field_names->set(sqlite3_column_name(srec->stmt, i), i);
                zc_dict_set(srec->field_names, sqlite3_column_name(srec->stmt, i), 0, (void*)i); 
            }
        }
        return 1;
    }else if (ret == SQLITE_DONE) {
        return ZC_OK;
    }
    ZCINFO("step error: %d", ret);
    return ZC_ERR;
}

int  
zc_sqliterec_field_int_pos(zcDBRec *rec, int pos, int defv)
{
    zcSQLiteRec *srec = (zcSQLiteRec*)rec;
    if (srec->field_types[pos] != SQLITE_INTEGER)
        return defv;
    return sqlite3_column_int(srec->stmt, pos);
}

int  
zc_sqliterec_field_int(zcDBRec *rec, const char *name, int defv)
{
    zcSQLiteRec *srec = (zcSQLiteRec*)rec;
    //int pos = field_names->get(name, -1);
    long pos = (long)zc_dict_get(srec->field_names, name, 0, (void*)-1);
    if (pos == -1)
        return defv;
    return zc_sqliterec_field_int_pos(rec, pos, defv);
}

int64_t 
zc_sqliterec_field_int64_pos(zcDBRec *rec, int pos, int64_t defv)
{
    zcSQLiteRec *srec = (zcSQLiteRec*)rec;
    if (srec->field_types[pos] != SQLITE_INTEGER)
        return defv;
    return sqlite3_column_int64(srec->stmt, pos);
}

int64_t 
zc_sqliterec_field_int64(zcDBRec *rec, const char *name, int64_t defv)
{
    zcSQLiteRec *srec = (zcSQLiteRec*)rec;
    long pos = (long)zc_dict_get(srec->field_names, name, 0, (void*)-1);
    if (pos == -1)
        return defv;
    return zc_sqliterec_field_int64_pos(rec, pos, defv);
}

double  
zc_sqliterec_field_float_pos(zcDBRec *rec, int pos, double defv)
{
    zcSQLiteRec *srec = (zcSQLiteRec*)rec;
    if (srec->field_types[pos] != SQLITE_INTEGER)
        return defv;
    return sqlite3_column_double(srec->stmt, pos);
}

double  
zc_sqliterec_field_float(zcDBRec *rec, const char *name, double defv)
{
    zcSQLiteRec *srec = (zcSQLiteRec*)rec;
    long pos = (long)zc_dict_get(srec->field_names, name, 0, (void*)-1);
    if (pos == -1)
        return defv;
    return zc_sqliterec_field_float_pos(rec, pos, defv);
}

const char* 
zc_sqliterec_field_str_pos(zcDBRec *rec, int pos, const char *defv)
{
    zcSQLiteRec *srec = (zcSQLiteRec*)rec;
    if (srec->field_types[pos] != SQLITE_TEXT)
        return defv;
    return (const char*)sqlite3_column_text(srec->stmt, pos);
}

const char* 
zc_sqliterec_field_str(zcDBRec *rec, const char *name, const char *defv)
{
    zcSQLiteRec *srec = (zcSQLiteRec*)rec;
    long pos = (long)zc_dict_get(srec->field_names, name, 0, (void*)-1);
    if (pos == -1)
        return defv;
    return zc_sqliterec_field_str_pos(rec, pos, defv);
}

const char* 
zc_sqliterec_field_blob_pos(zcDBRec *rec, int pos, int *len, const char *defv)
{
    zcSQLiteRec *srec = (zcSQLiteRec*)rec;
    if (srec->field_types[pos] != SQLITE_BLOB)
        return defv;
    *len = sqlite3_column_bytes(srec->stmt, pos);
    return (const char*)sqlite3_column_blob(srec->stmt, pos);
}

const char* 
zc_sqliterec_field_blob(zcDBRec *rec, const char *name, int *len, const char *defv)
{
    zcSQLiteRec *srec = (zcSQLiteRec*)rec;
    long pos = (long)zc_dict_get(srec->field_names, name, 0, (void*)-1);
    if (pos == -1)
        return defv;
    return zc_sqliterec_field_blob_pos(rec, pos, len, defv);
}

int   
zc_sqliterec_fetchone(zcDBRec *rec, const char *format, ...)
{
    zcSQLiteRec *srec = (zcSQLiteRec*)rec;
    va_list arg;
    int i;
    int length = strlen(format);
    va_start(arg, format);
    for (i = 0; i < length; i++) {
        switch(format[i]) {
            case 'i': {
                int *v = va_arg(arg, int*);
                *v = sqlite3_column_int(srec->stmt, i);
                break;
            }
            case 'l': {
                int64_t *v = va_arg(arg, int64_t*);
                *v = sqlite3_column_int64(srec->stmt, i);
                break;
            }
            case 'f':{
                double *v = va_arg(arg, double*);
                *v = sqlite3_column_double(srec->stmt, i);
                break;
            }
            case 's': {
                char *v = va_arg(arg, char*);
                char *r = (char*)sqlite3_column_text(srec->stmt, i);
                if (r) {
                    strcpy(v, r);
                }else{
                    v[0] = 0;
                }
                break;
            }
            case 'x':
                // ignore
                break;
        }
    }
    va_end(arg);
    return 0;
}

/*int   
zc_sqliterec_fetchall(zcDBRec *rec, const char *format)
{
    zcSQLiteRec *srec = (zcSQLiteRec*)rec;
    if (srec->data)
        return srec->rows;

    int flen = strlen(format);
    int _fields[100];
    int fsize = 0;

    int i, n=0;
    for (i=0; i<flen; i++) {
        switch(format[i]) {
        case 'i':
            fsize += 4;
            _fields[i] = n;
            srec->data_format[n] = format[i];
            n++;
            break;
        case 'I':
        case 'f':
            fsize += 8;
            _fields[i] = n;
            srec->data_format[n] = format[i];
            n++;
            break;
        case 's':
        case 'b':
            _fields[i] = n;
            srec->data_format[n] = format[i];
            n++;
            break;
        }
    }
    srec->data_format[n] = 0;

    srec->data = zc_array_new(100, sizeof(void*));
    //srec->reset();
    zc_sqliterec_reset(rec);
    srec->rows = 0;

    int rown = 0;
    // headsize: blocksize + field1 pos + field2 pos ...
    int headsize = (flen+1) * sizeof(int);
    ZCINFO("headsize:%d, flen:%d\n", headsize, flen);
    //while (srec->row_next() == 0) {
    while (zc_sqliterec_row_next(rec) == 0) {
        int size = fsize;
        for (i=0; i<flen; i++) {
            if (format[i] == 's') {
                size += sqlite3_column_bytes(srec->stmt, i) + 1;
            }
        }
        size += headsize;
        char *row = (char*)zc_malloc(size);
        memset(row, 0, size);

        int  *rowhead = (int*)row;
        rowhead[0] = size;
        char *rowdata = row + headsize;
        int ri = 0;
        n = 0;
        for (i = 0; i < flen; i++) {
            int lastri = ri;
            switch(format[i]) {
                case 'i': {
                    int v = sqlite3_column_int(srec->stmt, i);
                    memcpy(&rowdata[ri], &v, sizeof(int));
                    ri += sizeof(int);
                    n++;
                    break;
                }
                case 'I': {
                    int64_t v = sqlite3_column_int64(srec->stmt, i);
                    memcpy(&rowdata[ri], &v, sizeof(int64_t));
                    ri += sizeof(int64_t);
                    n++;
                    break;
                }
                case 'f':{
                    double v = sqlite3_column_double(srec->stmt, i);
                    memcpy(&rowdata[ri], &v, sizeof(double));
                    ri += sizeof(double);
                    n++;
                    break;
                }
                case 's': {
                    char *v = (char*)sqlite3_column_text(srec->stmt, i);
                    int vlen = sqlite3_column_bytes(srec->stmt, i)+1;
                    memcpy(&rowdata[ri], v, vlen);
                    ri += vlen;
                    n++;
                    break;
                }
                case 'b': {
                    char *v = (char*)sqlite3_column_blob(srec->stmt, i);
                    int vlen = sqlite3_column_bytes(srec->stmt, i);
                    memcpy(&rowdata[ri], v, vlen);
                    ri += vlen;
                    n++;
                    break;
                }
            }
            if (ri > lastri) {
                rowhead[n] = headsize + lastri;
                ZCINFO("row index: %d, %d\n", n, headsize + lastri);
            }
        }
        //data->append(row);
        if (rown == srec->data->size) {
            srec->data = zc_array_expand(srec->data, srec->data->size);
        }
        zc_array_set(srec->data, rown, row);
        rown++;
    }
    return srec->rows;
}

void* 
zc_sqliterec_get_pos(zcDBRec *rec, int rowid, int pos, void *defv)
{
    zcSQLiteRec *srec = (zcSQLiteRec*)rec;
    if (NULL == srec->data)
        return NULL;
    char *row = (char*)zc_array_get(srec->data, rowid, NULL);
    if (NULL == row) {
        return defv;
    }
    int *head = (int*)row;
    ZCINFO("head: %d %d %d %d\n", head[0], head[1], head[2], head[3]);
    return &row[head[pos+1]];
}

void* 
zc_sqliterec_get(zcDBRec *rec, int rowid, char *name, void *defv)
{
    zcSQLiteRec *srec = (zcSQLiteRec*)rec;
    long pos = (long)zc_dict_get(srec->field_names, name, 0, (void*)-1);
    if (pos == -1)
        return defv;
    return zc_sqliterec_get_pos(rec, rowid, pos, defv);
}*/


// ----- zcSQLiteDB
zcDB*    
zc_sqlitedb_new(const char *path)
{
    zcSQLiteDB *sdb = (zcSQLiteDB*)zc_malloc(sizeof(zcSQLiteDB));
    memset(sdb, 0, sizeof(zcSQLiteDB));

    strcpy(sdb->dbpath, path);
    int ret = sqlite3_open(path, &sdb->db);
    if (ret != SQLITE_OK) {
        ZCERROR("sqlite open error:%s\n", path);
        zc_free(sdb);
        return NULL;
    }
    
    sdb->del   = zc_sqlitedb_delete;
    sdb->exec  = zc_sqlitedb_exec;
    sdb->execf = zc_sqlitedb_execf;
    sdb->query = zc_sqlitedb_query;
    sdb->start = zc_sqlitedb_start;
    sdb->commit   = zc_sqlitedb_commit;
    sdb->rollback = zc_sqlitedb_rollback;
    sdb->last_insert_id = zc_sqlitedb_last_insert_id;

    return (zcDB*)sdb;
}
void     
zc_sqlitedb_delete(void *db)
{
    zcSQLiteDB *sdb = (zcSQLiteDB*)db;
    if (sdb->db) {
        sqlite3_close(sdb->db);
    } 
    zc_free(sdb);
}

int      
zc_sqlitedb_exec(zcDB *db, const char *sql)
{
    zcSQLiteDB *sdb = (zcSQLiteDB*)db; 
    int ret;
    char *errmsg;
    
    ret = sqlite3_exec(sdb->db, sql, NULL, NULL, &errmsg);
    if (ret != SQLITE_OK) {
        ZCWARN("sqlite exec err:%s\n", errmsg);
        sqlite3_free(errmsg);
        return -ret;
    }   
    return 0;
}

int      
zc_sqlitedb_execf(zcDB *db, const char *sql, const char *format, ...)
{
    zcSQLiteDB *sdb = (zcSQLiteDB*)db; 
    sqlite3_stmt *stmt;
    va_list arg;
    int ret;

    ret = sqlite3_prepare_v2(sdb->db, sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        ZCWARN("prepare error:%d, %s\n", ret, sqlite3_errmsg(sdb->db));
        return -ret;
    }

    int length = strlen(format);
    int i;
    va_start(arg, format);
    for (i = 0; i < length; i++) {
        switch(format[i]) {
            case 'i': {
                sqlite3_bind_int(stmt, i+1, va_arg(arg,int));
                break;
            }
            case 'l': {
                sqlite3_bind_int64(stmt, i+1, va_arg(arg,int64_t));
                break;
            }
            case 'f':{
                sqlite3_bind_double(stmt, i+1, va_arg(arg,double));
                break;
            }
            case 's': {
                sqlite3_bind_text(stmt, i+1, va_arg(arg,char*), -1, NULL);
                break;
            }
            case 'b': {
                sqlite3_bind_blob(stmt, i+1, va_arg(arg,char*), -1, NULL);
                break;
            }
        }
    }
    va_end(arg);
    ret = sqlite3_step(stmt);
    if (ret != SQLITE_OK && ret != SQLITE_DONE) {
        return -ret;
    }
    sqlite3_finalize(stmt);
    return ZC_OK;
}

zcDBRec* 
zc_sqlitedb_query(zcDB *db, const char *sql, const char *format, ...)
{
    zcSQLiteDB *sdb = (zcSQLiteDB*)db; 
    sqlite3_stmt *stmt;
    va_list arg;
    int ret;

    //ZCINFO("sql:%s", sql);
    ret = sqlite3_prepare_v2(sdb->db, sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        ZCWARN("prepare error:%d, %s\n", ret, sqlite3_errmsg(sdb->db));
        return NULL;
    }
    if (stmt == NULL) {
        ZCWARN("sqlite stmt is NULL");
        return NULL;
    }

    if (format) {
        int length = strlen(format);
        int i;
        va_start(arg, format);
        for (i = 0; i < length; i++) {
            switch(format[i]){
            case 'i': {
                sqlite3_bind_int(stmt, i+1, va_arg(arg,int));
                break;
            }
            case 'l': {
                sqlite3_bind_int64(stmt, i+1, va_arg(arg,int64_t));
                break;
            }
            case 'f': {
                sqlite3_bind_double(stmt, i+1, va_arg(arg,double));
                break;
            }
            case 's': {
                sqlite3_bind_text(stmt, i+1, va_arg(arg,char*), -1, NULL);
                break;
            }
            case 'b': {
                sqlite3_bind_blob(stmt, i+1, va_arg(arg,char*), -1, NULL);
                break;
            }

            }
        }
        va_end(arg);
    }
    return (zcDBRec*)zc_sqliterec_new(stmt);
}

int      
zc_sqlitedb_start(zcDB *db)
{
    return zc_sqlitedb_exec(db, "START");
}

int      
zc_sqlitedb_commit(zcDB *db)
{
    return zc_sqlitedb_exec(db, "COMMIT");
}

int      
zc_sqlitedb_rollback(zcDB *db)
{
    return zc_sqlitedb_exec(db, "ROLLBACK");
}

int64_t
zc_sqlitedb_last_insert_id(zcDB *db)
{
    zcSQLiteDB *sdb = (zcSQLiteDB*)db; 
    return sqlite3_last_insert_rowid(sdb->db);
}

#endif
