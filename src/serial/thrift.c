#include <zocle/zocle.h>
#include <zocle/serial/thrift.h>

void
zc_thrift_write_framed_begin(zcBuffer *buf)
{
    zc_buffer_clear(buf);
    zc_buffer_append(buf, "\0\0\0\0", 4);
}

void
zc_thrift_write_framed_end(zcBuffer *buf)
{
    char *data = buf->data;
    int n = zc_htob32(zc_buffer_used(buf) - 4);
    memcpy(data, &n, 4);
}

int
zc_thrift_write_msg_begin(zcBuffer *buf, char *name, char type, int seq)
{
    //zc_thrift_write_binary(buf, name, strlen(name));
    //zc_thrift_write_byte(buf, type);
    //zc_thrift_write_i32(buf, seq);

	zc_thrift_write_i32(buf, ZC_THRIFT_VERSION_1 | type);
    zc_thrift_write_binary(buf, name, strlen(name));
    zc_thrift_write_i32(buf, seq);
		
    return 0;
}

/*int
zc_thrift_write_msg_end(zcBuffer *buf)
{
    return zc_thrift_write_field_stop(buf);
}*/

int
zc_thrift_write_struct_begin(zcBuffer *buf, short id, char *name)
{
    return zc_thrift_write_field_begin(buf, name, ZC_THRIFT_STRUCT, 0);
}

/*int
zc_thrift_write_struct_end(zcBuffer *buf)
{
    return zc_thrift_write_field_stop(buf);
}*/

int
zc_thrift_write_field_begin(zcBuffer *buf, char *name, char type, short id)
{
    //zc_thrift_write_byte(buf, type);
    //zc_thrift_write_i16(buf, id);
	
	zc_buffer_append(buf, &type, 1);
	short id2 = zc_htob16(id);
	zc_buffer_append(buf, &id2, sizeof(short));
    return 0;
}

/*int
zc_thrift_write_field_end(zcBuffer *buf)
{
    return 0;
}

int
zc_thrift_write_field_stop(zcBuffer *buf)
{
	return zc_buffer_append(buf, ZC_THRIFT_STOP, 1);
}
*/


int
zc_thrift_write_map_begin(zcBuffer *buf, short id, char ktype, char vtype, int size)
{
	zc_thrift_write_field_begin(buf, NULL, ZC_THRIFT_MAP, id);
	
	zc_buffer_append(buf, &ktype, 1);
	zc_buffer_append(buf, &vtype, 1);
	zc_thrift_write_i32(buf, size);
    return 0;
}

/*int
zc_thrift_write_map_end(zcBuffer *buf)
{
    return 0;
}*/

int
zc_thrift_write_list_begin(zcBuffer *buf, short id, char etype, int size)
{
	zc_thrift_write_field_begin(buf, NULL, ZC_THRIFT_LIST, id);

	zc_buffer_append(buf, &etype, 1);
	zc_thrift_write_i32(buf, size);

    return 0;
}

/*int
zc_thrift_write_list_end(zcBuffer *buf)
{
    return 0;
}*/

int
zc_thrift_write_set_begin(zcBuffer *buf, short id, char etype, int size)
{
	zc_thrift_write_field_begin(buf, NULL, ZC_THRIFT_SET, id);

	zc_buffer_append(buf, &etype, 1);
	zc_thrift_write_i32(buf, size);

    return 0;
}

/*int
zc_thrift_write_set_end(zcBuffer *buf)
{
    return 0;
}*/

int
zc_thrift_write_field_bool(zcBuffer *buf, short id, int b)
{
	zc_thrift_write_field_begin(buf, NULL, ZC_THRIFT_BOOL, id);

	char v = 1;
    if (b > 0) {
		return zc_buffer_append(buf, &v, 1);
    }else{
		v = 0;
		return zc_buffer_append(buf, &v, 1);
    }
}


int
zc_thrift_write_field_byte(zcBuffer *buf, short id, char byte)
{
	zc_thrift_write_field_begin(buf, NULL, ZC_THRIFT_BYTE, id);
    return zc_buffer_append(buf, &byte, 1);
}

int
zc_thrift_write_field_i16(zcBuffer *buf, short id, short n)
{
	zc_thrift_write_field_begin(buf, NULL, ZC_THRIFT_I16, id);
	short n2 = zc_htob16(n);
    return zc_buffer_append(buf, &n2, sizeof(short));
}

int
zc_thrift_write_field_i32(zcBuffer *buf, short id, int n)
{
	zc_thrift_write_field_begin(buf, NULL, ZC_THRIFT_I32, id);
	int n2 = zc_htob32(n);
    return zc_buffer_append(buf, &n2, sizeof(int));
}

int
zc_thrift_write_field_i64(zcBuffer *buf, short id, long long  n)
{
	zc_thrift_write_field_begin(buf, NULL, ZC_THRIFT_I64, id);
	long long n2 = zc_htob64(n);
    return zc_buffer_append(buf, &n2, sizeof(long long));
}

int
zc_thrift_write_field_double(zcBuffer *buf, short id, double n)
{
	zc_thrift_write_field_begin(buf, NULL, ZC_THRIFT_DOUBLE, id);
    return zc_buffer_append(buf, &n, sizeof(double));
}

int
zc_thrift_write_field_binary(zcBuffer *buf, short id, char *s, int n)
{
	zc_thrift_write_field_begin(buf, NULL, ZC_THRIFT_STRING, id);
	int n2 = zc_htob32(n);
    zc_buffer_append(buf, &n2, sizeof(int));
    zc_buffer_append(buf, s, n);
    return 0;
}

// --- 
int
zc_thrift_write_bool(zcBuffer *buf, int b)
{
	char v = 1;
    if (b > 0) {
		return zc_buffer_append(buf, &v, 1);
    }else{
		v = 0;
		return zc_buffer_append(buf, &v, 1);
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
	short n2 = zc_htob16(n);
    return zc_buffer_append(buf, &n2, sizeof(short));
}

int
zc_thrift_write_i32(zcBuffer *buf, int n)
{
	int n2 = zc_htob32(n);
    return zc_buffer_append(buf, &n2, sizeof(int));
}

int
zc_thrift_write_i64(zcBuffer *buf, long long  n)
{
	long long n2 = zc_htob64(n);
    return zc_buffer_append(buf, &n2, sizeof(long long));
}

int
zc_thrift_write_double(zcBuffer *buf, double n)
{
    return zc_buffer_append(buf, &n, sizeof(double));
}

int
zc_thrift_write_binary(zcBuffer *buf, char *s, int n)
{
	int n2 = zc_htob32(n);
    zc_buffer_append(buf, &n2, sizeof(int));
    zc_buffer_append(buf, s, n);
    return 0;
}


int	 
zc_thrift_write_exception(zcBuffer *buf, char *s, int type, char *name, int seqid, bool framed)
{
	if (framed) {
		zc_thrift_write_framed_begin(buf);
	}
	zc_thrift_write_msg_begin(buf, name, ZC_THRIFT_EXCEPTION, seqid);
	zc_thrift_write_field_binary(buf, 1, s, strlen(s));
	zc_thrift_write_field_i32(buf, 2, type);
	zc_thrift_write_flag_end(buf);

	if (framed) {
		zc_thrift_write_framed_end(buf);
	}
	
	return ZC_OK;	
}

int
zc_thrift_write_flag_end(zcBuffer *buf)
{
	char type = ZC_THRIFT_STOP;
	return zc_buffer_append(buf, &type, 1);
}


// ---- read ----


int
zc_thrift_read_framed(const char *s, int *size)
{
	return zc_thrift_read_i32(s, size);
}

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
zc_thrift_read_struct_begin(const char *s, short *id)
{
	char type;
    return zc_thrift_read_field_begin(s, NULL, &type, id);
}

int
zc_thrift_read_field_begin(const char *s, char *name, char *type, short *id)
{
	memcpy(type, s, sizeof(const char));
    if (type == ZC_THRIFT_STOP) {
        return 1;
    }
    //_read_i16(s+1, id);
    memcpy(id, s+1, sizeof(short));
	*id = zc_htob16(*id);
 
    return 3;
}


int
zc_thrift_read_map_begin(const char *s, short *id, char *ktype, char *vtype, int *size)
{
	int n = 0;
	char type;
    n += zc_thrift_read_field_begin(s, NULL, &type, id);

	memcpy(ktype, s+n, sizeof(char));
	n++;
	memcpy(vtype, s+n, sizeof(char));
	n++;
    zc_thrift_read_i32(s+n, size);
	n += 4;

    return n;
}

int
zc_thrift_read_list_begin(const char *s, short *id, char *etype, int *size)
{
	int n = 0;
	char type;
    n += zc_thrift_read_field_begin(s, NULL, &type, id);

	memcpy(etype, s+n, sizeof(char));
	n++;
    n += zc_thrift_read_i32(s+n, size);

    return n;
}

int
zc_thrift_read_set_begin(const char *s, short *id, char *etype, int *size)
{
	int n = 0;
	char type;
    n += zc_thrift_read_field_begin(s, NULL, &type, id);

	memcpy(etype, s+n, sizeof(char));
	n++;
    n += zc_thrift_read_i32(s+n, size);

    return n;
}

int
zc_thrift_read_field_bool(const char *s, short *id, char *b)
{
	int n = 0;
	char type;
    n += zc_thrift_read_field_begin(s, NULL, &type, id);

    memcpy(b, s+n, sizeof(const char));
    return n+1;
}

int
zc_thrift_read_field_byte(const char *s, short *id, char *b)
{
	int n = 0;
	char type;
    n += zc_thrift_read_field_begin(s, NULL, &type, id);

    memcpy(b, s+n, sizeof(const char));
    return n+1;
}

int
zc_thrift_read_field_i16(const char *s, short *id, short *v)
{
	int n = 0;
	char type;
    n += zc_thrift_read_field_begin(s, NULL, &type, id);

    memcpy(v, s+n, sizeof(short));
	*v = zc_htob16(*v);
    return n+2;
}

int
zc_thrift_read_field_i32(const char *s, short *id, int *v)
{
	int n = 0;
	char type;
    n += zc_thrift_read_field_begin(s, NULL, &type, id);

    memcpy(v, s+n, sizeof(int));
	*v = zc_htob32(*v);
    return n+4;
}

int
zc_thrift_read_field_i64(const char *s, short *id, long long *v)
{
	int n = 0;
	char type;
    n += zc_thrift_read_field_begin(s, NULL, &type, id);

    memcpy(v, s+n, sizeof(long long));
	*v = zc_htob64(*v);
    return n+8;
}

int
zc_thrift_read_field_double(const char *s, short *id, double *v)
{
	int n = 0;
	char type;
    n += zc_thrift_read_field_begin(s, NULL, &type, id);

    memcpy(v, s+n, sizeof(double));
    return n+4;
}

int
zc_thrift_read_field_binary(const char *s, short *id, char *b, int *len)
{
	int n = 0;
	char type;
    n += zc_thrift_read_field_begin(s, NULL, &type, id);

    zc_thrift_read_i32(s+n, len);
	n += 4;
    memcpy(b, s+n, *len);
	n += *len;
    
    return n;
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
zc_thrift_read_i16(const char *s, short *v)
{
    memcpy(v, s, sizeof(short));
	*v = zc_htob16(*v);
    return 2;
}

int
zc_thrift_read_i32(const char *s, int *v)
{
    memcpy(v, s, sizeof(int));
	*v = zc_htob32(*v);
    return 4;
}

int
zc_thrift_read_i64(const char *s, long long *v)
{
    memcpy(v, s, sizeof(long long));
	*v = zc_htob64(*v);
    return 8;
}

int
zc_thrift_read_double(const char *s, double *v)
{
    memcpy(v, s, sizeof(double));
    return 4;
}

int
zc_thrift_read_binary(const char *s, char *b, int *len)
{
	int n;
    memcpy(&n, s, sizeof(int));
	n = zc_htob32(n);
    memcpy(b, s+4, n);
	*len = n;

    return 4+n;
}

int
zc_thrift_read_flag_end(const char *s)
{
    return 1;
}











