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


// 基本数据格式:
// 有两种:
// 1. 包长度(4字节，framed才有) + 消息版本类型(4字节) + 消息名长度(4字节) + 消息名(长度在前面) +  请求号(4字节) + 数据
// 2. 包长度(4字节，framed才有) + 消息名长度(4字节) + 消息名(长度在前面) +  消息类型(1字节) + 请求号(4字节) + 数据
// ---- write ----
// 简单数据(bool/byte/i16/i32/i64/double/string/binary)都由一个 field_begin(类型+编号) + 数据(write_xxx) 组成
// struct由 field_begin(类型+编号) + 数据(字段1) + 数据(字段2) + ... + end标识 组成
// map由 field_begin(类型+编号) + map_begin(key/val类型+个数) + 数据 + 数据 + ...  组成
// list由 field_begin(类型+编号) + list_begin(数据类型+个数) + 数据 + 数据 + ... 组成
// set由 field_begin(类型+编号) + set_begin(数据类型+个数) + 数据 + 数据 + ... 组成
// 
/*	
example:

zc_thrift_write_framed_head
zc_thrift_msg_begin
  zc_thrift_write_field_binary(xxx)
  zc_thrift_write_list_begin(xxx) // element is i32, length 2
    zc_thrift_write_i32
    zc_thrift_write_i32
  zc_thrift_write_set_begin(xxx) // elemtn is i64, length 3
    zc_thrift_write_i64
    zc_thrift_write_i64
    zc_thrift_write_i64
  zc_thrift_write_map_begin // key is string, value is string, length 2
    zc_thrift_write_binary  // key
    zc_thrift_write_binary  // value
    zc_thrift_write_binary  // key
    zc_thrift_write_binary  // value
  zc_thrift_write_struct  // struct has data id with type i32
    zc_thrift_write_field_i32
  zc_thrift_flag_end
zc_thrift_flag_end
zc_thrift_write_framed
*/
void zc_thrift_write_framed_begin(zcBuffer *buf);
void zc_thrift_write_framed_end(zcBuffer *buf);
int  zc_thrift_write_msg_begin(zcBuffer *buf, char *name, char type, int seq);
int  zc_thrift_write_struct_begin(zcBuffer *buf, short id, char *name);
int  zc_thrift_write_field_begin(zcBuffer *buf, char *name, char type, short id);
int  zc_thrift_write_map_begin(zcBuffer *buf, short id, char ktype, char vtype, int size);
int  zc_thrift_write_list_begin(zcBuffer *buf, short id, char etype, int size);
int  zc_thrift_write_set_begin(zcBuffer *buf, short id, char etype, int size);
int  zc_thrift_write_field_bool(zcBuffer *buf, short id, int b);
int  zc_thrift_write_field_byte(zcBuffer *buf, short id, char byte);
int  zc_thrift_write_field_i16(zcBuffer *buf, short id, short n);
int  zc_thrift_write_field_i32(zcBuffer *buf, short id, int n);
int  zc_thrift_write_field_i64(zcBuffer *buf, short id, long long  n);
int  zc_thrift_write_field_double(zcBuffer *buf, short id, double n);
int  zc_thrift_write_field_binary(zcBuffer *buf, short id, char *s, int n);

int  zc_thrift_write_bool(zcBuffer *buf, int b);
int  zc_thrift_write_byte(zcBuffer *buf, char byte);
int  zc_thrift_write_i16(zcBuffer *buf, short n);
int  zc_thrift_write_i32(zcBuffer *buf, int n);
int  zc_thrift_write_i64(zcBuffer *buf, long long  n);
int  zc_thrift_write_double(zcBuffer *buf, double n);
int  zc_thrift_write_binary(zcBuffer *buf, char *s, int n);

// only struct/msg has end flag
int  zc_thrift_write_flag_end(zcBuffer *buf);

int	 zc_thrift_write_exception(zcBuffer *buf, char *s, int type, char *name, int seqid, bool framed);

// ---- read ----

int  zc_thrift_read_framed(const char *s, int *size);
int  zc_thrift_read_msg_begin(const char *s, char *name, char *type, int *seqid);
int  zc_thrift_read_struct_begin(const char *s, short *id);
int  zc_thrift_read_field_begin(const char *s, char *name, char *type, short *id);
int  zc_thrift_read_map_begin(const char *s, short *id, char *ktype, char *vtype, int *size);
int  zc_thrift_read_list_begin(const char *s, short *id, char *etype, int *size);
int  zc_thrift_read_set_begin(const char *s, short *id, char *etype, int *size);

int  zc_thrift_read_field_bool(const char *s, short *id, char *b);
int  zc_thrift_read_field_byte(const char *s, short *id, char *b);
int  zc_thrift_read_field_i16(const char *s, short *id, short *n);
int  zc_thrift_read_field_i32(const char *s, short *id, int *n);
int  zc_thrift_read_field_i64(const char *s, short *id, long long *n);
int  zc_thrift_read_field_double(const char *s, short *id, double *n);
int  zc_thrift_read_field_binary(const char *s, short *id, char *b, int *n);

int  zc_thrift_read_bool(const char *s, char *b);
int  zc_thrift_read_byte(const char *s, char *b);
int  zc_thrift_read_i16(const char *s, short *n);
int  zc_thrift_read_i32(const char *s, int *n);
int  zc_thrift_read_i64(const char *s, long long *n);
int  zc_thrift_read_double(const char *s, double *n);
int  zc_thrift_read_binary(const char *s, char *b, int *n);


int  zc_thrift_read_flag_end(const char *s);


#endif

