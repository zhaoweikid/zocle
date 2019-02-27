#ifdef ZOCLE_WITH_MYSQL
#ifndef ZOCLE_DB_MYSQLDB_H
#define ZOCLE_DB_MYSQLDB_H

#include <stdio.h>
#include <stdint.h>
#include <zocle/db/dbif.h>
#include <zocle/ds/dict.h>
#include <mysql.h>

typedef struct zc_mysqlconf_t
{
	char host[16];
	int  port;
	char user[64];
	char pass[64];
	char charset[16];
	char db[64];
	int  conn_timeout;
	int  read_timeout;
	int  write_timeout;
}zcMySQLConf;

void zc_mysqlcf_init(zcMySQLConf *cf, char *host, int port, char *user, char *pass, char *db, char *charset);
void zc_mysqlcf_print(zcMySQLConf *cf);

typedef struct zc_mysqlrec_t
{
	ZC_DBREC_IF
	MYSQL_RES   *res;
    MYSQL_ROW   row;
    int         err;  // error number
    uint64_t    _pos;
    uint64_t    _rows;
    int         _fields;
    int         _get_row_next;
	zcDict      *fieldmap;
}zcMySQLRec;

zcDBRec* zc_mysqlrec_new(void *stmt);
void zc_mysqlrec_delete(void *rec);

//void zc_mysqlrec_close(zcDBRec *rec);
//void zc_mysqlrec_clear(zcDBRec *rec);
int  zc_mysqlrec_reset(zcDBRec *rec);
int  zc_mysqlrec_row_next(zcDBRec *rec);

int  zc_mysqlrec_field_int_pos(zcDBRec *rec, int pos, int defv);
int  zc_mysqlrec_field_int(zcDBRec *rec, const char*, int defv);
int64_t zc_mysqlrec_field_int64_pos(zcDBRec *rec, int pos, int64_t defv);
int64_t zc_mysqlrec_field_int64(zcDBRec *rec, const char*, int64_t defv);
double  zc_mysqlrec_field_float_pos(zcDBRec *rec, int pos, double defv);
double  zc_mysqlrec_field_float(zcDBRec *rec, const char*, double defv);
const char* zc_mysqlrec_field_str_pos(zcDBRec *rec, int pos, const char *defv);
const char* zc_mysqlrec_field_str(zcDBRec *rec, const char*, const char *defv);
const char* zc_mysqlrec_field_blob_pos(zcDBRec *rec, int pos, int *len, const char *defv);
const char* zc_mysqlrec_field_blob(zcDBRec *rec, const char*, int *len, const char *defv);

int   zc_mysqlrec_fetchone(zcDBRec *rec, const char *format, ...);
int   zc_mysqlrec_fetchall(zcDBRec *rec, const char *format);
void* zc_mysqlrec_get_pos(zcDBRec *rec, int rowid, int pos, void *defv);
void* zc_mysqlrec_get(zcDBRec *rec, int rowid, char *name, void *defv);


typedef struct zc_mysqldb_t
{
	ZC_DB_IF

	zcMySQLConf conf;
	MYSQL   *_mysql;
    char    _isopen;
	int		*err;
}zcMySQLDB;

zcDB*    zc_mysqldb_new(zcMySQLConf *cf);
void     zc_mysqldb_delete(void *);
int      zc_mysqldb_exec(zcDB *db, const char *sql);
int      zc_mysqldb_execf(zcDB *db, const char *sql, const char *format, ...);
zcDBRec* zc_mysqldb_query(zcDB *db, const char *sql, const char *format, ...);
int      zc_mysqldb_open(zcDB *db);
int      zc_mysqldb_start(zcDB *db);
int      zc_mysqldb_commit(zcDB *db);
int      zc_mysqldb_rollback(zcDB *db);
int64_t  zc_mysqldb_last_insert_id(zcDB *db);


#endif
#endif
