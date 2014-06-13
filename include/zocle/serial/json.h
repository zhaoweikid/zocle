#ifndef ZOCLE_SERIAL_JSON_H
#define ZOCLE_SERIAL_JSON_H

#include <stdio.h>
#include <zocle/str/string.h>
#include <zocle/base/object.h>

int		  zc_json_pack(zcString *s, zcObject *obj);
int		  zc_json_unpack(zcObject **obj, zcString *s);
zcString* zc_json_pack_obj(zcObject *obj);
zcObject* zc_json_unpack_obj(zcString *s);
int		  zc_json_check(zcString *);

#endif
