#include "mqtc370.h"

/* mqtc_process_pubrec()
 * Process PUBREC from client for QOS 2 PUBLISH message.
 */

int mqtc_process_pubrec(MQTC *mqtc, CPB *pb)
{
    int             rc      = 4;
    unsigned        len     = 0;
    unsigned        pos     = 0;
    unsigned short  msgid;

    if (!pb) goto quit;
    
    // mqtc_packet_dump(mqtc, pb, "PUBREC");

    /* decode variable header */
    if (!pb->varpos) {
        rc = util_length_decode(pb->buf, &len);
        if (!rc) goto quit;
        /* start of variable header */
        pb->varpos  = (unsigned short)rc;
    }
    pos = pb->varpos;

    rc += 4;
    
    /* Get msgid */
    msgid = util_get_ushort(&pb->buf[pos]);
    pos += 2;

    // wtof("%s: msgid=%04X (%u)", __func__, msgid, msgid);

    /* create PUBREL packet */
    pb              = calloc(1, sizeof(CPB) + 4);
    if (!pb) goto quit;
    
    pb->bufsize     = 4;
    pb->buf[0]      = TYPE_PUBREL | 0x02; /* QOS 1 flag set */
    pb->buf[1]      = 2;    /* size of msgid that follows */
    pos             = 2;

    util_put_ushort(msgid, &pb->buf[pos]);
    pos += 2;

    /* save this packet for retry */
    mqtc_packet_retry(mqtc, msgid, pb);

    /* add this packet to the send queue */
    rc = mqtc_session_send(mqtc, pb);

quit:
    // wtof("%s: exit rc=%d", __func__, rc);
    return rc;
}
