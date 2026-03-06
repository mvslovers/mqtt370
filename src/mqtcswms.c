#include "mqtc370.h"

int mqtc_set_will_msg(MQTC *mqtc, const char *will_msg)
{
    int     rc      = 8;

    util_free_char(&mqtc->will_msg);
    mqtc->will_len = 0;
    
    if (will_msg) {
        mqtc->will_len = strlen(will_msg);
        mqtc->will_msg = util_dup_e2a(will_msg, mqtc->will_len);
        if (!mqtc->will_msg) {
            mqtc->will_len = 0;
            goto quit;
        }
    }
    
    rc = 0;

quit:
    return rc;
}
