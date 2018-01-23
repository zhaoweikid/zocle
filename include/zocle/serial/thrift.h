#ifndef __ZC_THRIFT_H__
#define __ZC_THRIFT_H__

#include <stdio.h>
#include <stdint.h>


#define ZC_THRIFT_VERSION_MASK 0xffff0000
#define ZC_THRIFT_VERSION_1    0x80010000
#define ZC_THRIFT_TYPE_MASK    0x000000ff

#define ZC_THRIFT_STOP		0
#define ZC_THRIFT_VOID		1
#define ZC_THRIFT_BOOL		2
#define ZC_THRIFT_BYTE		3
#define ZC_THRIFT_I08		3
#define ZC_THRIFT_DOUBLE	4
#define ZC_THRIFT_I16		6
#define ZC_THRIFT_I32		8
#define ZC_THRIFT_I64		10
#define ZC_THRIFT_STRING	11
#define ZC_THRIFT_UTF7		11
#define ZC_THRIFT_STRUCT	12
#define ZC_THRIFT_MAP		13
#define ZC_THRIFT_SET		14
#define ZC_THRIFT_LIST		15
#define ZC_THRIFT_UTF8		16
#define ZC_THRIFT_UTF16		17

#define ZC_THRIFT_CALL		1
#define ZC_THRIFT_REPLY		2
#define ZC_THRIFT_EXCEPTION	3
#define ZC_THRIFT_ONEWAY	4	

#define ZC_THRIFT_ERR_UNKNOWN			0
#define ZC_THRIFT_ERR_UNKNOWN_METHOD	1
#define ZC_THRIFT_ERR_INVALID_MSG_TYPE	2
#define ZC_THRIFT_ERR_WRONG_MSG_NAME	3
#define ZC_THRIFT_ERR_BAD_SEQ_ID		4
#define ZC_THRIFT_ERR_MISS_RESULT		5
#define ZC_THRIFT_ERR_INTERNAL			6
#define ZC_THRIFT_ERR_PROTOCOL			7
#define ZC_THRIFT_ERR_INVALID_TRANSFORM	8
#define ZC_THRIFT_ERR_INVALID_PROTOCOL	9
#define ZC_THRIFT_ERR_UNSUPPORTED_CLIENT_TYPE	10



// ---- write ----

void zc_thrift_write_framed_head(zcBuffer *buf);
void zc_thrift_write_framed(zcBuffer *buf);
int  zc_thrift_write_msg_begin(zcBuffer *buf, char *name, char type, int seq);
int  zc_thrift_write_msg_end(zcBuffer *buf);
int  zc_thrift_write_struct_begin(zcBuffer *buf, char *name);
int  zc_thrift_write_struct_end(zcBuffer *buf);
int  zc_thrift_write_field_begin(zcBuffer *buf, char *name, char type, int id);
//int  zc_thrift_write_field_end(zcBuffer *buf);
int  zc_thrift_write_field_stop(zcBuffer *buf);
int  zc_thrift_write_map_begin(zcBuffer *buf, char ktype, char vtype, int size);
//int  zc_thrift_write_map_end(zcBuffer *buf);
int  zc_thrift_write_list_begin(zcBuffer *buf, char etype, int size);
//int  zc_thrift_write_list_end(zcBuffer *buf);
int  zc_thrift_write_set_begin(zcBuffer *buf, char etype, int size);
//int  zc_thrift_write_set_end(zcBuffer *buf);
int  zc_thrift_write_bool(zcBuffer *buf, int b);
int  zc_thrift_write_byte(zcBuffer *buf, char byte);
int  zc_thrift_write_i16(zcBuffer *buf, short n);
int  zc_thrift_write_i32(zcBuffer *buf, int n);
int  zc_thrift_write_i64(zcBuffer *buf, long long  n);
int  zc_thrift_write_double(zcBuffer *buf, double n);
int  zc_thrift_write_binary(zcBuffer *buf, char *s, int n);

int	 zc_thrift_write_exception(zcBuffer *buf, char *s, int type, char *name, int seqid, bool framed);

// ---- read ----

int  zc_thrift_read_msg_begin(const char *s, char *name, char *type, int *seqid);
int  zc_thrift_read_msg_end(const char *s);
int  zc_thrift_read_struct_begin(const char *s);
int  zc_thrift_read_struct_end(const char *s);
int  zc_thrift_read_field_begin(const char *s, char *name, char *type, short *id);
//int  zc_thrift_read_field_end(const char *s);
int  zc_thrift_read_map_begin(const char *s, char *ktype, char *vtype, int *size);
//int  zc_thrift_read_map_end(const char *s);
int  zc_thrift_read_list_begin(const char *s, char *etype, int *size);
//int  zc_thrift_read_list_end(const char *s);
int  zc_thrift_read_set_begin(const char *s, char *etype, int *size);
//int  zc_thrift_read_set_end(const char *s);
int  zc_thrift_read_bool(const char *s, char *b);
int  zc_thrift_read_byte(const char *s, char *b);
int  zc_thrift_read_i16(const char *s, short *n);
int  zc_thrift_read_i32(const char *s, int *n);
int  zc_thrift_read_i64(const char *s, long long *n);
int  zc_thrift_read_double(const char *s, double *n);
int  zc_thrift_read_binary(const char *s, char *b, int *n);


#endif
