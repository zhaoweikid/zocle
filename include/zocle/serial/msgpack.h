#ifndef ZOCLE_SERIAL_MSGPACK_H
#define ZOCLE_SERIAL_MSGPACK_H

#include <zocle/str/string.h>
#include <zocle/base/object.h>

zcObject*	zc_msgpack_loads(zcString*);
zcString*	zc_msgpack_dumps(zcObject*);


#endif
