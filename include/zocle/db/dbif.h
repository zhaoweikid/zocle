#ifndef ZOCLE_DB_DBIF_H
#define ZOCLE_DB_DBIF_H

#include <stdio.h>
#include <stdint.h>

#define ZC_DBREC_IF	\
	void (*del)(void *rec);\
	int  (*reset)(zcDBRec *rec);\
	int  (*row_next)(zcDBRec *rec);\
	int  (*field_int_pos)(zcDBRec *rec, int pos, int defv);\
	int  (*field_int)(zcDBRec *rec, const char*, int defv);\
	int64_t (*field_int64_pos)(zcDBRec *rec, int pos, int64_t defv);\
	int64_t (*field_int64)(zcDBRec *rec, const char*, int64_t defv);\
	double  (*field_float_pos)(zcDBRec *rec, int pos, double defv);\
	double  (*field_float)(zcDBRec *rec, const char*, double defv);\
	const char* (*field_str_pos)(zcDBRec *rec, int pos, const char *defv);\
	const char* (*field_str)(zcDBRec *rec, const char*, const char *defv);\
	const char* (*field_blob_pos)(zcDBRec *rec, int pos, int *len, const char *defv);\
	const char* (*field_blob)(zcDBRec *rec, const char*, int *len, const char *defv);\
	int (*fetchone)(zcDBRec *rec, const char *format, ...);\

	/*int (*fetchall)(zcDBRec *rec, const char *format);\
	void* (*get_pos)(zcDBRec *rec, int rowid, int pos, void *defv);\
	void* (*get)(zcDBRec *rec, int rowid, char *name, void *defv);*/


#define ZC_DB_IF \
	void (*del)(void *rec);\
	int (*exec)(zcDB *db, const char *sql);\
    int (*execf)(zcDB *db, const char *sql, const char *format, ...);\
    zcDBRec* (*query)(zcDB *db, const char *sql, const char *format, ...);\
    int (*start)(zcDB *db);\
    int (*commit)(zcDB *db);\
    int (*rollback)(zcDB *db);\
    int64_t (*last_insert_id)(zcDB *db);

struct __zc_dbrec;
typedef struct __zc_dbrec zcDBRec;

struct __zc_dbrec
{
	ZC_DBREC_IF
};

struct __zc_db;
typedef struct __zc_db zcDB;

struct __zc_db
{
	ZC_DB_IF
};

#endif
