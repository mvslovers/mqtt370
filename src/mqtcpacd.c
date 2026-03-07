#include "mqtc370.h"

int mqtc_packet_discard(MQTC *mqtc, unsigned short msgid)
{
    int         rc      = 0;
    RPB         *rpb;
    unsigned    n, count;
    
    lock(&mqtc->rpbq, LOCK_EXC);
    count = array_count(&mqtc->rpbq);
    for(n=count; n > 0; n--) {
        rpb = array_get(&mqtc->rpbq, n);

        if (!rpb) continue;
        
        // wtodumpf(rpb, sizeof(RPB), "%s: RPB", __func__);
        if (rpb->msgid == msgid) {
            // wtof("%s: discard msgid=0x%04X", __func__, msgid);
            array_del(&mqtc->rpbq, n);
            if (rpb->cpb) free(rpb->cpb);
            free(rpb);
        }
    }
    unlock(&mqtc->rpbq, LOCK_EXC);
    
    return rc;
}
