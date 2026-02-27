#include "mqttutil.h"

unsigned short util_get_ushort(const unsigned char *uc)
{
    unsigned short  n;
    
    n = uc[0];
    n = n << 8;
    n += uc[1];

    // wtof("%s: uc=%02X%02X n=%04X", __func__, uc[0], uc[1], n);
    
    return n;
}
