#include "mqtc370.h"

void mqtc_free_client(MQTC **pmqtc)
{
    MQTC        *mqtc;
    unsigned    n, count;
    
    if (!pmqtc) goto quit;
    
    mqtc = *pmqtc;
    if (!mqtc) goto quit;

    if (mqtc->sock >= 0 || mqtc->task) {
        mqtc_close_client(mqtc);
    }

    /* reset client (packet buffers) */
    mqtc_reset_client(mqtc);

    /* release resource not processed by reset */
    util_free_char(&mqtc->broker_host);
    util_free_char(&mqtc->broker_port);
    util_free_char(&mqtc->userid);
    util_free_char(&mqtc->password);
    util_free_char(&mqtc->will_topic);
    util_free_char(&mqtc->will_msg);

    free(mqtc);
    *pmqtc = NULL;

quit:
    return;
}
