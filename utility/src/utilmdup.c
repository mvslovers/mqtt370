#include "mqttutil.h"

char *util_memdup(unsigned char *uc, unsigned len)
{
    char        *mem    = calloc(1, len);
   
    if (!mem) goto quit;
    
    memcpy(mem, uc, len);

quit:
    return mem;
}
