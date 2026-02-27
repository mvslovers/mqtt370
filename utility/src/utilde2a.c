#include "mqttutil.h"

char *util_dup_e2a(const unsigned char *uc, unsigned len)
{
    char        *str    = calloc(1, len + 1);
    unsigned    n;
    
    if (!str) goto quit;
    
    for(n=0; n < len; n++) {
        str[n] = utile2a[uc[n]];
    }

quit:
    return str;
}
