#include "mqtc370.h"

int mqtc_set_clientid(MQTC *mqtc, const char *clientid)
{
    int     rc      = 8;

    util_free_char(&mqtc->clientid);
    
    if (clientid) {
        mqtc->clientid = strdup(clientid);
        if (!mqtc->clientid) goto quit;
    }
    
    rc = 0;

quit:
    return rc;
}
