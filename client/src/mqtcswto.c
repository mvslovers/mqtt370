#include "mqtc370.h"

int mqtc_set_will_topic(MQTC *mqtc, const char *will_topic)
{
    int     rc      = 8;

    util_free_char(&mqtc->will_topic);
    
    if (will_topic) {
        mqtc->will_topic = strdup(will_topic);
        if (!mqtc->will_topic) goto quit;
    }
    
    rc = 0;

quit:
    return rc;
}
