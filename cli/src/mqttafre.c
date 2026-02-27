#include "mqttcli.h"

void mqtt_args_free(char ***pargv)
{
    char        **argv;
    unsigned    n, count;
    
    if (!pargv) goto quit;

    argv = *pargv;
    if (!argv) goto quit;
    
    count = array_count(pargv);
    for(n=count; n > 0; n--) {
        char    *p = array_del(pargv, n);
        
        if (!p) continue;
        free(p);
    }
    
    array_free(pargv);

quit:
    return;
}
