#include <zocle/serial/json.h>
#include <zocle/ds/list.h>
#include <zocle/ds/dict.h>
#include <zocle/base/defines.h>
#include <zocle/serial/json.h>
#include <zocle/ds/hashtable.h>
#include <zocle/ds/list.h>
#include <zocle/str/string.h>
#include <zocle/mem/alloc.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

static int
_json_pack(zcString *s, zcObject *obj)
{
    switch(obj->__type) {
    case ZC_INT8:
        zc_str_append_format(s, "%d", ((zcInt8*)obj)->v);
        break;
    case ZC_INT16:
        zc_str_append_format(s, "%d", ((zcInt16*)obj)->v);
        break;
    case ZC_INT32:
        zc_str_append_format(s, "%d", ((zcInt32*)obj)->v);
        break;
    case ZC_UINT8:
        zc_str_append_format(s, "%u", ((zcUInt8*)obj)->v);
        break;
    case ZC_UINT16:
        zc_str_append_format(s, "%u", ((zcUInt16*)obj)->v);
        break;
    case ZC_UINT32:
        zc_str_append_format(s, "%u", ((zcUInt32*)obj)->v);
        break;
    case ZC_INT64:
        zc_str_append_format(s, "%lld", ((zcInt64*)obj)->v);
        break;
    case ZC_UINT64:
        zc_str_append_format(s, "%llu", ((zcUInt64*)obj)->v);
        break;
    case ZC_DOUBLE:
        zc_str_append_format(s, "%f", ((zcDouble*)obj)->v);
        break;
    case ZC_BOOL:
        if (((zcBool*)obj)->v) {
            zc_str_append(s, "true");
        }else{
            zc_str_append(s, "false");
        }
        break;
    case ZC_NULL:
        zc_str_append(s, "null");
        break;
    case ZC_STRING: {
        zc_str_append_c(s, '"');
        zc_str_append_escape(s, ((zcString*)obj)->data);
        zc_str_append_c(s, '"');
        break;
    }
    case ZC_LIST: {
        zcList   *list = (zcList*)obj;
        zcObject *item;
        zcListNode *node;
        int i = 0;
        zc_str_append(s, "[");
        zc_list_foreach(list, node) {
            item = (zcObject*)node->data;
            if (_json_pack(s, item) < 0) {
                return ZC_ERR;
            }
            i++;
            if (i < list->size) {
                zc_str_append(s, ",");
            }
        }
        zc_str_append(s, "]");
        break;
    }
    case ZC_DICT: {
        const char *key;
        zcObject   *item;
        int  i = 0;
        zcDict *ht = (zcDict*)obj;
        zc_str_append(s, "{");
        zc_dict_foreach_start(ht, key, item)
            zc_str_append_c(s, '"');
            zc_str_append_escape(s, key);
            zc_str_append(s, "\":");
            if (_json_pack(s, item) < 0)
                return ZC_ERR;
            i++;
            if (i < ht->len) {
                zc_str_append(s, ",");
            }
        zc_dict_foreach_end
        zc_str_append(s, "}");
        break;
    }
    default:
        ZCWARN("json pack type error:%d", obj->__type);
        return ZC_ERR;
        //break;
    }
    return ZC_OK;
}


int 
zc_json_pack(zcString *s, zcObject *obj)
{
    return _json_pack(s, obj);
}

zcString*
zc_json_pack_obj(zcObject *obj)
{
    zcString *s = zc_str_new(64);
    int ret = _json_pack(s, obj);
    if (ret < 0) {
        zc_str_delete(s);
        return NULL;
    }
    return s;
}

// ------ unpack json ------
typedef struct {
    char      c;
    zcObject *obj;
}zcJsonInfoItem;

typedef struct {
    zcJsonInfoItem *data;
    int count;
    int pos;
    //zcObject *obj;
}zcJsonInfo;

static int _json_unpack(zcString *s, int *i, zcJsonInfo *info, zcObject **obj);

/*static int
_json_unpack_add2parent(zcObject *x, char *key, zcJsonInfo *info)
{
    zcObject *last = info->data[info->pos-1].obj;
    switch (last->__type) {
        case ZC_DICT: {
            if (key) {
                zc_dict_add_str((zcDict*)last, key, (void*)x); 
            }
            break;
        }
        case ZC_LIST: {
            zc_list_append((zcList*)last, x);
            break;
        }
        default: {
            break;
        }
    }
    return ZC_OK;
}*/

static int
_json_unpack_push(zcObject *x, char *key, zcJsonInfo *info)
{
    if (info->pos >= info->count) {
        ZCERROR("json parse error: buffer too small");
        return ZC_ERR;
    }
    //ZCINFO("add2parent: %d, key:%p", info->pos, key);
    if (info->pos == 0) {
        info->data[0].c = 0;
        info->data[0].obj = x;
        info->pos++;
    }else if (info->pos > 0) {
        if (x->__type == ZC_DICT) {
            info->data[info->pos].c = '{';
            info->data[info->pos].obj = x;
            info->pos++;
        }else if (x->__type == ZC_LIST) {
            info->data[info->pos].c = '[';
            info->data[info->pos].obj = x;
            info->pos++;
        }
        /*if (link) {
            _json_unpack_add2parent(x, key, info);
        }*/
    }
    return ZC_OK;
}


static void
_json_unpack_pop(zcJsonInfo *info)
{
    if (info->pos > 1) {
        info->pos--;
    }
}

static int
_json_unpack_string(zcString *s, int *pos, zcJsonInfo *info, zcString **strobj)
{
    //ZCINFO("=== try string");
    const char *data = s->data;
    int i = *pos;
    
    zcString *value;
    if (strobj == NULL || *strobj == NULL) {
        value = zc_str_new(16);
        int ret = _json_unpack_push((zcObject*)value, NULL, info);
        if (ret < 0) {
            ZCERROR("json parse error at: %d", i);
            return ret;
        }
    }else{
        value = *strobj;
    }
    //ZCINFO("value size:%d", value->size);
    i++; // skip "
    while (i < s->len && data[i] != '"') {
        if (data[i] != '\\') {
            zc_str_append_c(value, data[i]);
            i++;
            continue;
        }
        i++;
        if (i >= s->len)
            return ZC_ERR;
        switch(data[i]) {
        case '"': 
            zc_str_append_c(value, '"');
            break;
        case '\\':
            zc_str_append_c(value, '\\');
            break;
        case '/': 
            zc_str_append_c(value, '/');
            break;
        case 'b':
            zc_str_append_c(value, '\b');
            break;
        case 'f':
            zc_str_append_c(value, '\f');
            break;
        case 'n':
            zc_str_append_c(value, '\n');
            break;
        case 'r':
            zc_str_append_c(value, '\r');
            break;
        case 't':
            zc_str_append_c(value, '\t');
            break;
        case 'u': {
            if (i+4 >= s->len) {
                ZCERROR("json parse error at: %d", i);
                return ZC_ERR;
            }
            // char buf[16] = {0};
            // _ucs2_to_utf8(data[i], buf);
            //zc_str_append(value, buf);
            i += 3; // 本来应该+=4, 但后面还有个++，这里只+=3
            break;
        }
        default:
            zc_str_append_c(value, data[i]);
            break;
        }
        i++;
    }
    if (data[i] != '"') {
        ZCERROR("json parse error at: %d", i);
        return ZC_ERR;
    }
    i++; // skip "
    *pos = i;

    if (strobj)
        *strobj = value;
    return i;
}

static int
_json_unpack_number(zcString *s, int *pos, zcJsonInfo *info, zcObject **obj)
{
    //ZCINFO("=== try number");
    const char *data = s->data;
    int ret;
    int i = *pos;

    char buf[128] = {0}; 
    int  bufi = 0;
    zcObject *x = NULL;

    if (data[i] == '-') {
        buf[bufi] = '-';
        bufi++;
        i++;
    }

    while (i < s->len) {
        if (data[i] < '0' || data[i] > '9') {
            //ZCERROR("json parse error at: %d, char:%c", i, data[i]);
            //return ZC_ERR;
            if (i==0 || (i==1 && data[0] == '-')) {
                ZCERROR("json parse error at: %d, char:%c", i, data[i]);
                return ZC_ERR;
            }
            break;
        }
        buf[bufi] = data[i];
        bufi++;
        i++;
    }

    if (data[i] != '.') {
        buf[bufi] = 0;
        x = (zcObject*)zc_int64_new((int64_t)strtoll(buf, NULL, 10));
        ret = _json_unpack_push(x, NULL, info);
        if (ret < 0) {
            ZCERROR("json parse error at: %d", i);
            return ret;
        }
        *pos = i;
        if (obj)
            *obj = x;
        return i;
    }
    buf[bufi] = '.';
    bufi++;
    i++; // skip .

    while (i < s->len) {
        //ZCINFO("float %d, %c", i, data[i]);
        if (data[i] < '0' || data[i] > '9') {
            //ZCERROR("json parse error at: %d", i);
            //return ZC_ERR;
            if (data[i-1] == '.') {
                ZCERROR("json parse error at: %d", i);
                return ZC_ERR;
            }
            buf[bufi] = 0;
            break;
        }
        buf[bufi] = data[i];
        bufi++;
        i++;
    }
    //ZCINFO("number buf:%s", buf);
    x = (zcObject*)zc_double_new((double)strtold(buf, NULL));
    if (obj) {
        *obj = x;
    }else{
        ret = _json_unpack_push(x, NULL, info);
        if (ret < 0) {
            ZCERROR("json parse error at: %d", i);
            return ret;
        }
    }

    *pos = i;

    return i;
}

static int
_json_unpack_dict(zcString *s, int *pos, zcJsonInfo *info, zcObject **obj)
{
    //ZCINFO("=== try dict");
    const char *data = s->data;
    int ret;
    int i = *pos;
    int retcode = 0;

    zcDict *x = zc_dict_new_full(16, 0, zc_free_func, zc_obj_delete);
    ret = _json_unpack_push((zcObject*)x, NULL, info);
    if (ret < 0) {
        ZCERROR("json parse error at:%d", i);
        zc_dict_delete(x);
        return ret;
    }

    zcString *key   = zc_str_new(16);
    zcObject *value = NULL;
    i++; // skip {
    
    while (data[i] != '}') {
        zc_str_clear(key);
        value = NULL;
        while (data[i] == ' ') i++;
        ret = _json_unpack_string(s, &i, info, &key);
        if (ret < 0) {
            retcode = ret;
            goto dict_error;
        }
        //ZCINFO("key: %s", key->data);
        while (data[i] == ' ') i++;
        if (data[i] != ':') {
            ZCERROR("json parse error at: %d", i);
            retcode = ZC_ERR;
            goto dict_error;
        }
        i++; // skip :
        while (data[i] == ' ') i++;

        ret = _json_unpack(s, &i, info, &value);
        if (ret < 0) {
            ZCERROR("json parse error at: %d", i);
            retcode = ret;
            goto dict_error;
        }
        
        zcString *output = zc_str_new(1024);
        zc_obj_format(value, output);
        //ZCINFO("add key:%s value:%s", key->data, output->data);
        zc_str_delete(output);

        zc_dict_add_str(x, key->data, value);

        while (data[i] == ' ') i++;
        if (data[i] != ',' && data[i] != '}') {
            ZCERROR("json parse error at: %d", i);
            retcode = -1;
            goto dict_error;
        }
        if (data[i] == ',') {
            i++;
        }
    }
    i++; // skip }
    _json_unpack_pop(info);
    *pos = i;

    if (obj) {
        *obj = (zcObject*)x;
    }
    zc_str_delete(key);
    return i;

dict_error:
    zc_str_delete(key);
    return retcode;
}

static int
_json_unpack_list(zcString *s, int *pos, zcJsonInfo *info, zcObject **obj)
{
    //ZCINFO("=== try list");
    const char *data = s->data;
    int ret;
    int i = *pos;
    zcList *x = zc_list_new();
    x->del = zc_obj_delete;
    ret = _json_unpack_push((zcObject*)x, NULL, info);
    if (ret < 0) {
        ZCERROR("json parse error at:%d", i);
        return ret;
    }

    zcObject *value = NULL;
    i++; // skip [
    while (data[i] != ']') {
        while (i<s->len && data[i] == ' ') i++;
        ret = _json_unpack(s, &i, info, &value);
        if (ret < 0) {
            ZCERROR("json parse error at: %d", i);
            return ZC_ERR;
        }

        /*zcString *output = zc_str_new(1024);
        zc_obj_format(value, output);
        ZCINFO("list value type:%d data:%s", x->__type, output->data);
        zc_str_delete(output);*/

        zc_list_append(x, value);
        value = NULL;
        while (i<s->len && data[i] == ' ') i++;
        if (data[i] != ',' && data[i] != ']') {
            ZCERROR("json parse error at: %d", i);
            return ZC_ERR;
        }
        if (data[i] == ',') {
            i++;
        }
    }
    i++; // skip ]
    _json_unpack_pop(info);
    *pos = i;

    if (obj) {
        *obj = (zcObject*)x;
    }
    return i;
}

static int
_json_unpack(zcString *s, int *pos, zcJsonInfo *info, zcObject **obj)
{
    int  ret;
    int  i = *pos;
    zcObject *x = NULL;
    const char *data = s->data;

    if (i >= s->len) {
        return ZC_ERR;
    }
    switch(data[i]) {
    case '{': {
        ret = _json_unpack_dict(s, &i, info, obj);
        if (ret < 0) {
            ZCERROR("json parse error at:%d", i);
            return ret;
        }
        break;
    }
    case '[': {
        ret = _json_unpack_list(s, &i, info, obj);
        if (ret < 0) {
            ZCERROR("json parse error at:%d", i);
            return ret;
        }
        break;
    }
    case '"':
        ret = _json_unpack_string(s, &i, info, (zcString**)obj);
        if (ret < 0) {
            ZCERROR("json parse error at:%d", i);
            return ZC_ERR;
        }
        break;
    case 'n': // null
        //ZCINFO("=== try null");
        if (data[i+1] != 'u' || data[i+2] != 'l' || data[i+3] != 'l') {
            ZCERROR("json parse error at:%d", i);
            return ZC_ERR;
        }
        x = (zcObject*)zc_null_new();
        if (obj) {
            *obj = x;
        }else{
            ret = _json_unpack_push(x, NULL, info);
            if (ret < 0) { 
                ZCERROR("json parse error at:%d", i);
                return ret;
            }
        }
        i += 4;
        break;
    case 't': // true
        //ZCINFO("=== try true");
        if (data[i+1] != 'r' || data[i+2] != 'u' || data[i+3] != 'e') {
            ZCERROR("json parse error at:%d", i);
            return ZC_ERR;
        }
        x = (zcObject*)zc_bool_new(true);
        if (obj) {
            *obj = x;
        }else{
            ret = _json_unpack_push(x, NULL, info);
            if (ret < 0) { 
                ZCERROR("json parse error at:%d", i);
                return ret;
            }
        }
        i += 4;
        break;
    case 'f': // false
        //ZCINFO("=== try false");
        if (data[i+1] != 'a' || data[i+2] != 'l' || data[i+3] != 's' || data[i+4] != 'e') {
            ZCERROR("json parse error at:%d", i);
            return ZC_ERR;
        }
        x = (zcObject*)zc_bool_new(false);
        if (obj) {
            *obj = x;
        }else{
            ret = _json_unpack_push(x, NULL, info);
            if (ret < 0) { 
                ZCERROR("json parse error at:%d", i);
                return ret;
            }
        }
        i += 5;
        break;
    case '-':
    case '0' ... '9':
        ret = _json_unpack_number(s, &i, info, obj);
        if (ret < 0) {
            ZCERROR("json parse error at:%d", i);
            return ret;
        }
        break;
    default:
        ZCERROR("json parse error at:%d", i);
        return ZC_ERR;
    }
    *pos = i;
    return i;
}

int
zc_json_unpack(zcObject **obj, zcString *s)
{
    zcJsonInfoItem buf[256];
    memset(buf, 0, sizeof(zcJsonInfoItem)*256);

    zcJsonInfo info;
    info.data  = buf;
    info.count = 256;
    info.pos   = 0;
    
    int i = 0;
    int ret = _json_unpack(s, &i, &info, NULL);
    if (ret < 0)
        return ret;
    *obj = info.data[0].obj;
    return ZC_OK;
}

zcObject*
zc_json_unpack_obj(zcString *s)
{
    zcObject *obj = NULL;  
    int ret = zc_json_unpack(&obj, s);
    if (ret < 0)
        return NULL;
    return obj;
}

//---------------------------------

static int _json_check(zcString *s, int *pos, zcJsonInfo *info);

static int
_json_check_string(zcString *s, int *pos, zcJsonInfo *info)
{
    //ZCINFO("=== try string");
    const char *data = s->data;
    int i = *pos;
    
    /*if (strobj == NULL || *strobj == NULL) {
        int ret = _json_unpack_push((zcObject*)value, NULL, info);
        if (ret < 0) {
            ZCERROR("json parse error at: %d", i);
            return ret;
        }
    }*/
    //ZCINFO("value size:%d", value->size);
    i++; // skip "
    while (i < s->len && data[i] != '"') {
        if (data[i] != '\\') {
            i++;
            continue;
        }
        i++;
        if (i >= s->len)
            return ZC_ERR;
        switch(data[i]) {
        case 'u': {
            if (i+4 >= s->len) {
                ZCERROR("json parse error at: %d", i);
                return ZC_ERR;
            }
            i += 3; // 本来应该+=4, 但后面还有个++，这里只+=3
            break;
        }
        default:
            break;
        }
        i++;
    }
    if (data[i] != '"') {
        ZCERROR("json parse error at: %d", i);
        return ZC_ERR;
    }
    i++; // skip "
    *pos = i;

    return i;
}

static int
_json_check_number(zcString *s, int *pos, zcJsonInfo *info)
{
    //ZCINFO("=== try number");
    const char *data = s->data;
    //int ret;
    int i = *pos;

    if (data[i] == '-') {
        i++;
    }

    while (i < s->len) {
        if (data[i] < '0' || data[i] > '9') {
            //ZCERROR("json parse error at: %d, char:%c", i, data[i]);
            //return ZC_ERR;
            if (i==0 || (i==1 && data[0] == '-')) {
                ZCERROR("json parse error at: %d, char:%c", i, data[i]);
                return ZC_ERR;
            }
            break;
        }
        i++;
    }

    if (data[i] != '.') {
        return i;
    }
    i++; // skip .

    while (i < s->len) {
        if (data[i] < '0' || data[i] > '9') {
            if (data[i-1] == '.') {
                ZCERROR("json parse error at: %d", i);
                return ZC_ERR;
            }
            break;
        }
        i++;
    }
    //ZCINFO("number buf:%s", buf);
    *pos = i;

    return i;
}

static int
_json_check_dict(zcString *s, int *pos, zcJsonInfo *info)
{
    const char *data = s->data;
    int ret;
    int i = *pos;
    int retcode = 0;

    i++; // skip {
    
    while (data[i] != '}') {
        while (data[i] == ' ') i++;
        ret = _json_check_string(s, &i, info);
        if (ret < 0) {
            retcode = ret;
            goto dict_error;
        }
        //ZCINFO("key: %s", key->data);
        while (data[i] == ' ') i++;
        if (data[i] != ':') {
            ZCERROR("json parse error at: %d", i);
            retcode = ZC_ERR;
            goto dict_error;
        }
        i++; // skip :
        while (data[i] == ' ') i++;

        ret = _json_check(s, &i, info);
        if (ret < 0) {
            ZCERROR("json parse error at: %d", i);
            retcode = ret;
            goto dict_error;
        }
        
        while (data[i] == ' ') i++;
        if (data[i] != ',' && data[i] != '}') {
            ZCERROR("json parse error at: %d", i);
            retcode = -1;
            goto dict_error;
        }
        if (data[i] == ',') {
            i++;
        }
    }
    i++; // skip }
    *pos = i;
    return i;

dict_error:
    return retcode;
}

static int
_json_check_list(zcString *s, int *pos, zcJsonInfo *info)
{
    //ZCINFO("=== try list");
    const char *data = s->data;
    int ret;
    int i = *pos;

    i++; // skip [
    while (data[i] != ']') {
        while (i<s->len && data[i] == ' ') i++;
        ret = _json_check(s, &i, info);
        if (ret < 0) {
            ZCERROR("json parse error at: %d", i);
            return ZC_ERR;
        }

        while (i<s->len && data[i] == ' ') i++;
        if (data[i] != ',' && data[i] != ']') {
            ZCERROR("json parse error at: %d", i);
            return ZC_ERR;
        }
        if (data[i] == ',') {
            i++;
        }
    }
    i++; // skip ]
    *pos = i;
    return i;
}

static int
_json_check(zcString *s, int *pos, zcJsonInfo *info)
{
    int  ret;
    int  i = *pos;
    const char *data = s->data;

    if (i >= s->len) {
        return ZC_ERR;
    }
    switch(data[i]) {
    case '{': {
        ret = _json_check_dict(s, &i, info);
        if (ret < 0) {
            ZCERROR("json parse error at:%d", i);
            return ret;
        }
        break;
    }
    case '[': {
        ret = _json_check_list(s, &i, info);
        if (ret < 0) {
            ZCERROR("json parse error at:%d", i);
            return ret;
        }
        break;
    }
    case '"':
        ret = _json_check_string(s, &i, info);
        if (ret < 0) {
            ZCERROR("json parse error at:%d", i);
            return ZC_ERR;
        }
        break;
    case 'n': // null
        if (data[i+1] != 'u' || data[i+2] != 'l' || data[i+3] != 'l') {
            ZCERROR("json parse error at:%d", i);
            return ZC_ERR;
        }
        i += 4;
        break;
    case 't': // true
        if (data[i+1] != 'r' || data[i+2] != 'u' || data[i+3] != 'e') {
            ZCERROR("json parse error at:%d", i);
            return ZC_ERR;
        }
        i += 4;
        break;
    case 'f': // false
        if (data[i+1] != 'a' || data[i+2] != 'l' || data[i+3] != 's' || data[i+4] != 'e') {
            ZCERROR("json parse error at:%d", i);
            return ZC_ERR;
        }
        i += 5;
        break;
    case '-':
    case '0' ... '9':
        ret = _json_check_number(s, &i, info);
        if (ret < 0) {
            ZCERROR("json parse error at:%d", i);
            return ret;
        }
        break;
    default:
        ZCERROR("json parse error at:%d", i);
        return ZC_ERR;
    }
    *pos = i;
    return i;
}

int
zc_json_check(zcString *s)
{
    zcJsonInfoItem buf[256];
    memset(buf, 0, sizeof(zcJsonInfoItem)*256);

    zcJsonInfo info;
    info.data  = buf;
    info.count = 256;
    info.pos   = 0;
    
    int i = 0;
    int ret = _json_check(s, &i, &info);
    if (ret < 0)
        return ret;
    return ZC_OK;
}

