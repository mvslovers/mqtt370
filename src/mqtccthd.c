#include "mqtc370.h"

int mqtc_create_thread(MQTC *mqtc)
{
    int         rc          = 8;

    if (!mqtc) goto quit;

    if (!mqtc->task) {
        /* start the processing thread */
        mqtc->task_ecb      = 0;
        mqtc->task          = cthread_create_ex(mqtc_processing_thread, mqtc, NULL, 16*1024);
        if (!mqtc->task) {
            wtof("%s: unable to start processing thread", __func__);
            mqtc_close_client(mqtc);
            goto quit;
        }
    }

    rc = 0;

quit:
    return rc;
}
