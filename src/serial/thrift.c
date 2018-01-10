#include <zocle/zocle.h>
#include <zocle/serial/thrift.h>

void
zc_thrift_write_framed_head(zcBuffer *buf)
{
    zc_buffer_clear(buf);
    zc_buffer_append(buf, "\0\0\0\0", 4);
}

void
zc_thrift_write_framed(zcBuffer *buf)
{
    char *data = buf->data;
    int n = zc_buffer_used(buf) - 4;
    memcpy(data, &n, 4);
}

int
zc_thrift_write_msg_begin(zcBuffer *buf, char *name, char type, int seq)
{
    zc_thrift_write_binary(buf, name, strlen(name));
    zc_thrift_write_byte(buf, type);
    zc_thrift_write_i32(buf, seq);
    return 0;
}

int
zc_thrift_write_msg_end(zcBuffer *buf)
{
    return 0;
}

int
zc_thrift_write_struct_begin(zcBuffer *buf, char *name)
{
    return 0;
}

int
zc_thrift_write_struct_end(zcBuffer *buf)
{
    return 0;
}

int
zc_thrift_write_field_begin(zcBuffer *buf, char *name, char type, int id)
{
    zc_thrift_write_byte(buf, type);
    zc_thrift_write_i16(buf, id);
    return 0;
}

int
zc_thrift_write_field_end(zcBuffer *buf)
{
    return 0;
}

int
zc_thrift_write_field_stop(zcBuffer *buf)
{
    return zc_thrift_write_byte(buf, ZC_THRIFT_STOP);
}

int
zc_thrift_write_map_begin(zcBuffer *buf, char ktype, char vtype, int size)
{
    zc_thrift_write_byte(buf, ktype);
    zc_thrift_write_byte(buf, vtype);
    zc_thrift_write_i32(buf, size);
    return 0;
}

int
zc_thrift_write_map_end(zcBuffer *buf)
{
    return 0;
}

int
zc_thrift_write_list_begin(zcBuffer *buf, char etype, int size)
{
    zc_thrift_write_byte(buf, etype);
    zc_thrift_write_i32(buf, size);
    return 0;
}

int
zc_thrift_write_list_end(zcBuffer *buf)
{
    return 0;
}

int
zc_thrift_write_set_begin(zcBuffer *buf, char etype, int size)
{
    zc_thrift_write_byte(buf, etype);
    zc_thrift_write_i32(buf, size);
    return 0;
}

int
zc_thrift_write_set_end(zcBuffer *buf)
{
    return 0;
}

int
zc_thrift_write_bool(zcBuffer *buf, int b)
{
    if (b > 0) {
        return zc_thrift_write_byte(buf, 1);
    }else{
        return zc_thrift_write_byte(buf, 0);
    }
}

int
zc_thrift_write_byte(zcBuffer *buf, char byte)
{
    return zc_buffer_append(buf, &byte, 1);
}

int
zc_thrift_write_i16(zcBuffer *buf, short n)
{
    return zc_buffer_append(buf, &n, sizeof(short));
}

int
zc_thrift_write_i32(zcBuffer *buf, int n)
{
    return zc_buffer_append(buf, &n, sizeof(int));
}

int
zc_thrift_write_i64(zcBuffer *buf, long long  n)
{
    return zc_buffer_append(buf, &n, sizeof(long long));
}

int
zc_thrift_write_double(zcBuffer *buf, double n)
{
    return zc_buffer_append(buf, &n, sizeof(double));
}

int
zc_thrift_write_binary(zcBuffer *buf, char *s, int n)
{
    zc_buffer_append(buf, &n, 4);
    zc_buffer_append(buf, s, n);
    return 0;
}

// ---- read ----

int
zc_thrift_read_msg_begin(const char *s, char *name, char *type, int *seqid)
{
    int n = 0;
    int size = 0;
    zc_thrift_read_i32(s, &size);
    n += 4;
    if (size < 0) {
        int ver = size & ZC_THRIFT_VERSION_MASK;
        if (ver != ZC_THRIFT_VERSION_1)
            return ZC_ERR;

        *type = size & ZC_THRIFT_TYPE_MASK;
        int namelen = 0;
        n += zc_thrift_read_binary(s+n, name, &namelen);
        n += zc_thrift_read_i32(s+n, seqid); 
    }else{
        memcpy(name, s+n, size);
        name[size] = 0;
        n += size;
        n += zc_thrift_read_byte(s+n, type);
        n += zc_thrift_read_i32(s+n, seqid);
    }

    return n;
}

int
zc_thrift_read_msg_end(const char *s)
{
    return 0;
}

int
zc_thrift_read_struct_begin(const char *s)
{
    return 0;
}

int
zc_thrift_read_struct_end(const char *s)
{
    return 0;
}

int
zc_thrift_read_field_begin(const char *s, char *type, short *id)
{
    zc_thrift_read_byte(s, type);
    if (type == ZC_THRIFT_STOP) {
        return 1;
    }
    zc_thrift_read_i16(s+1, id);
    return 3;
}

int
zc_thrift_read_field_end(const char *s)
{
    return 0;
}

int
zc_thrift_read_map_begin(const char *s, char *ktype, char *vtype, int *size)
{
    zc_thrift_read_byte(s, ktype);
    zc_thrift_read_byte(s+1, vtype);
    zc_thrift_read_i32(s+2, size);

    return 6;
}

int
zc_thrift_read_map_end(const char *s)
{
    return 0;
}

int
zc_thrift_read_list_begin(const char *s, char *etype, int *size)
{
    zc_thrift_read_byte(s, etype);
    zc_thrift_read_i32(s+1, size);
    return 5;
}

int
zc_thrift_read_list_end(const char *s)
{
    return 0;
}

int
zc_thrift_read_set_begin(const char *s, char *etype, int *size)
{
    zc_thrift_read_byte(s, etype);
    zc_thrift_read_i32(s+1, size);
    return 5;
}

int
zc_thrift_read_set_end(const char *s)
{
    return 0;
}

int
zc_thrift_read_bool(const char *s, char *b)
{
    memcpy(b, s, sizeof(const char));
    return 1;
}

int
zc_thrift_read_byte(const char *s, char *b)
{
    memcpy(b, s, sizeof(const char));
    return 1;
}

int
zc_thrift_read_i16(const char *s, short *n)
{
    memcpy(n, s, sizeof(short));
    return 2;
}

int
zc_thrift_read_i32(const char *s, int *n)
{
    memcpy(n, s, sizeof(int));
    return 4;
}

int
zc_thrift_read_i64(const char *s, long long *n)
{
    memcpy(n, s, sizeof(long long));
    return 8;
}

int
zc_thrift_read_double(const char *s, double *n)
{
    memcpy(n, s, sizeof(double));
    return 4;
}

int
zc_thrift_read_binary(const char *s, char *b, int *n)
{
    zc_thrift_read_i32(s, n);
    memcpy(b, s+4, *n);
    
    return 4+(*n);
}











