#include "mqtc370.h"

int mqtc_reply_pubrec(MQTC *mqtc, unsigned msgid)
{
    int             rc      = 8;
    CPB             *pb     = calloc(1, sizeof(CPB) + 4);
    unsigned char   *uc     = ((unsigned char *)&msgid) + 2;

    // wtof("%s: mqtc=%p msgid=0x%04X waitresp=%u", 
    //     __func__, mqtc, msgid, mqtc->flags & MQTC_FLAG_WAITRESP);
        
    if (!pb) goto quit;
    
    pb->bufsize     = 4;
    pb->buf[0]      = TYPE_PUBREC;
    pb->buf[1]      = 2;    /* length of variable header */
    /* PUBREC Variable Header */
    pb->buf[2]      = uc[0];    /* msgid MSB    */
    pb->buf[3]      = uc[1];    /* msgid LSB    */

    // wtodumpf(pb->buf, pb->bufsize, "%s: PB", __func__);

    /* add this packet to the retry queue */
    mqtc_packet_retry(mqtc, msgid, pb);

    /* add this packet to the send queue */
    rc = mqtc_session_send(mqtc, pb);

quit:
    return rc;
}

