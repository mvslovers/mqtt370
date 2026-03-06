#include "mqtc370.h"

int mqtc_packet_retry(MQTC *mqtc, unsigned short msgid, CPB *pb)
{
    int         rc      = 8;
    RPB         *rpb;
    unsigned    pbsize;
    time64_t    now;

    /* remove any packet with the same message id from the retry queue */
    mqtc_packet_discard(mqtc, msgid);

    /* allocate a new retry record */
    rpb = calloc(1, sizeof(RPB));
    if (!rpb) goto quit;
    
    time64(&now);
    __64_add_u32(&now, 5, &rpb->retry);
    
    rpb->msgid  = msgid;
    
    pbsize = sizeof(CPB) + pb->bufsize;
    rpb->cpb = calloc(1, pbsize);
    if (!rpb->cpb) {
        free(rpb);
        goto quit;
    }
    
    memcpy(rpb->cpb, pb, pbsize);

    /* put this retry record in the retry packet queue */
    lock(&mqtc->rpbq, LOCK_EXC);
    // wtodumpf(rpb, sizeof(RPB), "%s: RPB msgid=0x%04X", __func__, msgid);
    array_add(&mqtc->rpbq, rpb);
    unlock(&mqtc->rpbq, LOCK_EXC);
    
    rc = 0;

quit:
    return rc;
}
