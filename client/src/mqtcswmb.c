#include "mqtc370.h"

int mqtc_set_will_msg_bin(MQTC *mqtc, const void *msg, unsigned len)
{
    int     rc      = 8;

    util_free_char(&mqtc->will_msg);
    mqtc->will_len = 0;
    
    if (msg && len) {
        mqtc->will_msg = calloc(1, len);
        if (!mqtc->will_msg) goto quit;

        memcpy(mqtc->will_msg, msg, len);
        mqtc->will_len = len;
    }
    
    rc = 0;

quit:
    return rc;
}
