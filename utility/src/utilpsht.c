#include "mqttutil.h"

void util_put_ushort(unsigned value, unsigned char *buf)
{
    unsigned char *uc = ((unsigned char*)&value) + 2;
    
    buf[0] = uc[0];
    buf[1] = uc[1];
}
