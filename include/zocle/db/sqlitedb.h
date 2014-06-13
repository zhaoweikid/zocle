#ifdef ZOCLE_WITH_SQLITE
#ifndef ZOCLE_DB_SQLITEDB_H
#define ZOCLE_DB_SQLITEDB_H

#include <stdio.h>
#include <sqlite3.h>
#include <stdint.h>
#include <limits.h>
#include <zocle/db/dbif.h>
#include <zocle/ds/array.h>
#include <zocle/ds/dict.h>

typedef struct __zc_sqliterec
{
	ZC_DBREC_IF

	int fields;
    int rows;

    int field_types[100];
    sqlite3_stmt *stmt;
    zcDict	     *field_names;
    zcArray		 *data;
    char data_format[100];
}zcSQLiteRec;

zcDBRec* zc_sqliterec_new(void *stmt);
void zc_sqliterec_delete(void *rec);

//void zc_sqliterec_close(zcDBRec *rec);
//void zc_sqliterec_clear(zcDBRec *rec);
int  zc_sqliterec_reset(zcDBRec *rec);
int  zc_sqliterec_row_next(zcDBRec *rec);

int  zc_sqliterec_field_int_pos(zcDBRec *rec, int pos, int defv);
int  zc_sqliterec_field_int(zcDBRec *rec, const char*, int defv);
int64_t zc_sqliterec_field_int64_pos(zcDBRec *rec, int pos, int64_t defv);
int64_t zc_sqliterec_field_int64(zcDBRec *rec, const char*, int64_t defv);
double  zc_sqliterec_field_float_pos(zcDBRec *rec, int pos, double defv);
double  zc_sqliterec_field_float(zcDBRec *rec, const char*, double defv);
const char* zc_sqliterec_field_str_pos(zcDBRec *rec, int pos, const char *defv);
const char* zc_sqliterec_field_str(zcDBRec *rec, const char*, const char *defv);
const char* zc_sqliterec_field_blob_pos(zcDBRec *rec, int pos, int *len, const char *defv);
const char* zc_sqliterec_field_blob(zcDBRec *rec, const char*, int *len, const char *defv);

int   zc_sqliterec_fetchone(zcDBRec *rec, const char *format, ...);

/*int   zc_sqliterec_fetchall(zcDBRec *rec, const char *format);
void* zc_sqliterec_get_pos(zcDBRec *rec, int rowid, int pos, void *defv);
void* zc_sqliterec_get(zcDBRec *rec, int rowid, char *name, void *defv);*/

typedef struct __zc_sqlitedb
{
	ZC_DB_IF

    char dbpath[PATH_MAX];
    sqlite3 *db;
}zcSQLiteDB;

zcDB*    zc_sqlitedb_new(const char *path);
void     zc_sqlitedb_delete(void *);
int      zc_sqlitedb_exec(zcDB *db, const char *sql);
int      zc_sqlitedb_execf(zcDB *db, const char *sql, const char *format, ...);
zcDBRec* zc_sqlitedb_query(zcDB *db, const char *sql, const char *format, ...);
int      zc_sqlitedb_start(zcDB *db);
int      zc_sqlitedb_commit(zcDB *db);
int      zc_sqlitedb_rollback(zcDB *db);
int64_t  zc_sqlitedb_last_insert_id(zcDB *db);


#endif
#endif
