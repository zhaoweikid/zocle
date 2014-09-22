#include <zocle/serial/msgpack.h>

zcString*	zc_msgpack_pack_obj(zcObject *obj)
{
    zcObject *tmp = obj;

    while (tmp) {
    switch (obj->__type) {
    case ZC_INT8:
    case ZC_INT16:
    case ZC_INT32:
    case ZC_INT64:
        break;
    case ZC_UINT8:
    case ZC_UINT16:
    case ZC_UINT32:
    case ZC_UINT64:
        break;

    case ZC_DOUBLE:
        break;
    case ZC_NULL:
        break;
    case ZC_BOOL:
        break;
    case ZC_LIST:
        break;
    case ZC_DICT:
        break;

    default:
        break;
    }
    }
    return NULL;
}

zcObject*	zc_msgpack_unpack_obj(zcString *s)
{
    return NULL;
}



