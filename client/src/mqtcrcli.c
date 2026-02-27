#include "mqtc370.h"

void mqtc_reset_client(MQTC *mqtc)
{
    unsigned    n, count;
    
    if (!mqtc) goto quit;

    util_free_char((char**)&mqtc->rpb);
    util_free_char((char**)&mqtc->spb);

    if (mqtc->spbq) {
        count = array_count(&mqtc->spbq);
        for(n = count; n > 0; n--) {
            char *p = (char*) array_del(&mqtc->spbq, n);
            if (!p) continue;
            util_free_char(&p);
        }
        array_free(&mqtc->spbq);
    }

    if (mqtc->rpbq) {
        count = array_count(&mqtc->rpbq);
        for(n = count; n > 0; n--) {
            RPB *rpb = (RPB*) array_del(&mqtc->rpbq, n);
            if (!rpb) continue;
            if (rpb->cpb) free(rpb->cpb);
            free(rpb);
        }
        array_free(&mqtc->rpbq);
    }

quit:
    return;
}
