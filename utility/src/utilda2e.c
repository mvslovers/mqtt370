#include "mqttutil.h"

char *util_dup_a2e(const unsigned char *uc, unsigned len)
{
    char        *str    = calloc(1, len + 1);
    unsigned    n;
    
    if (!str) goto quit;
    
    for(n=0; n < len; n++) {
        str[n] = utila2e[uc[n]];
    }

quit:
    return str;
}
