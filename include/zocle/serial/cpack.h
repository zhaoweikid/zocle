#ifndef ZOCLE_SERIAL_CPACK_H 
#define ZOCLE_SERIAL_CPACK_H

#include <stdio.h>
#include <zocle/str/string.h>

int zc_cpack(zcString *, char *format, ...);
int zc_cpack_append(zcString *, char *format, ...);
int zc_cpack_end(zcString *, int);

int zc_cunpack(char *buf, int buflen, char *format, ...);

//int zc_cpack_obj(zcString*, char *format, ...);
//int zc_cunpack_obj(zcString*, int headlen, char *format, ...);

#endif
