#ifndef __THRIFT_H__
#define __THRIFT_H__

#include <stdio.h>
#include <stdint.h>


#define THRIFT_VERSION_MASK -65536
#define THRIFT_VERSION_1    -2147418112
#define THRIFT_TYPE_MASK    0x000000ff

#define THRIFT_STOP 0
#define THRIFT_VOID 1
#define THRIFT_BOOL 2
#define THRIFT_BYTE 3
#define THRIFT_I08  3
#define THRIFT_DOUBLE   4
#define THRIFT_I16  6
#define THRIFT_I32  8
#define THRIFT_I64  10
#define THRIFT_STRING   11
#define THRIFT_UTF7 11
#define THRIFT_MAP  13
#define THRIFT_SET  14
#define THRIFT_LIST 15
#define THRIFT_UTF8 16
#define THRIFT_UTF16    17


void zc_thrift_write_framed_head(zcBuffer *buf);
void zc_thrift_write_framed(zcBuffer *buf);
int  zc_thrift_write_msg_begin(zcBuffer *buf, char *name, char type, int seq);
int  zc_thrift_write_msg_end(zcBuffer *buf);
int  zc_thrift_write_struct_begin(zcBuffer *buf, char *name);
int  zc_thrift_write_struct_end(zcBuffer *buf);
int  zc_thrift_write_field_begin(zcBuffer *buf, char *name, char type, int id);
int  zc_thrift_write_field_end(zcBuffer *buf);
int  zc_thrift_write_field_stop(zcBuffer *buf);
int  zc_thrift_write_map_begin(zcBuffer *buf, char ktype, char vtype, int size);
int  zc_thrift_write_map_end(zcBuffer *buf);
int  zc_thrift_write_list_begin(zcBuffer *buf, char etype, int size);
int  zc_thrift_write_list_end(zcBuffer *buf);
int  zc_thrift_write_set_begin(zcBuffer *buf, char etype, int size);
int  zc_thrift_write_set_end(zcBuffer *buf);
int  zc_thrift_write_bool(zcBuffer *buf, int b);
int  zc_thrift_write_byte(zcBuffer *buf, char byte);
int  zc_thrift_write_i16(zcBuffer *buf, short n);
int  zc_thrift_write_i32(zcBuffer *buf, int n);
int  zc_thrift_write_i64(zcBuffer *buf, long long  n);
int  zc_thrift_write_double(zcBuffer *buf, double n);
int  zc_thrift_write_binary(zcBuffer *buf, char *s, int n);

// ---- read ----

int  zc_thrift_read_msg_begin(char *s, char *name, char *type, int *seqid);
int  zc_thrift_read_msg_end(char *s);
int  zc_thrift_read_struct_begin(char *s);
int  zc_thrift_read_struct_end(char *s);
int  zc_thrift_read_field_begin(char *s, char *name, char *type, short *id);
int  zc_thrift_read_field_end(char *s);
int  zc_thrift_read_map_begin(char *s, char *ktype, char *vtype, int *size);
int  zc_thrift_read_map_end(char *s);
int  zc_thrift_read_list_begin(char *s, char *etype, int *size);
int  zc_thrift_read_list_end(char *s);
int  zc_thrift_read_set_begin(char *s, char *etype, int *size);
int  zc_thrift_read_set_end(char *s);
int  zc_thrift_read_bool(char *s, char *b);
int  zc_thrift_read_byte(char *s, char *b);
int  zc_thrift_read_i16(char *s, short *n);
int  zc_thrift_read_i32(char *s, int *n);
int  zc_thrift_read_i64(char *s, long long *n);
int  zc_thrift_read_double(char *s, double *n);
int  zc_thrift_read_binary(char *s, char *b, int *n);


#endif
