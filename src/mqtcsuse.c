#include "mqtc370.h"

int mqtc_set_userid(MQTC *mqtc, const char *userid)
{
    int     rc      = 8;

    util_free_char(&mqtc->userid);
    
    if (userid) {
        mqtc->userid = strdup(userid);
        if (!mqtc->userid) goto quit;
    }
    
    rc = 0;

quit:
    return rc;
}
