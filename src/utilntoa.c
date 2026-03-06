#include "mqttutil.h"

int util_ntoa(unsigned addr, char *buf)
{
    unsigned char *p = (unsigned char*)&addr;

    return sprintf(buf, "%u.%u.%u.%u", p[0], p[1], p[2], p[3]);
}
