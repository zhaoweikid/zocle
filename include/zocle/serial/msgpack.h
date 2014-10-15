#ifndef ZOCLE_SERIAL_MSGPACK_H
#define ZOCLE_SERIAL_MSGPACK_H

#include <zocle/str/string.h>
#include <zocle/base/object.h>

zcString*	zc_msgpack_pack_obj(zcObject*);
zcObject*	zc_msgpack_unpack_obj(zcString*);


#endif
