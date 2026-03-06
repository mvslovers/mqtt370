#include "mqtc370.h"

int mqtc_set_password(MQTC *mqtc, const char *password)
{
    int     rc      = 8;

    util_free_char(&mqtc->password);
    
    if (password) {
        mqtc->password = strdup(password);
        if (!mqtc->password) goto quit;
    }
    
    rc = 0;

quit:
    return rc;
}
