#include "mqtc370.h"

/* mqtc_process_pubrel()
 * Process PUBREL from client for QOS 2 PUBLISH message.
 */

int mqtc_process_pubrel(MQTC *mqtc, CPB *pb)
{
    int             rc      = 4;
    unsigned        len     = 0;
    unsigned        pos     = 0;
    unsigned short  msgid;

    if (!pb) goto quit;
    
    // mqtb_packet_dump(mqtb, mqtbs, pb, "PUBREL");

    /* decode variable header */
    if (!pb->varpos) {
        rc = util_length_decode(pb->buf, &len);
        if (!rc) goto quit;
        /* start of variable header */
        pb->varpos  = (unsigned short)rc;
    }
    pos = pb->varpos;

    rc  += 4;
    
    /* Get msgid */
    msgid = util_get_ushort(&pb->buf[pos]);
    pos += 2;

    // wtof("%s: msgid=%04X (%u)", __func__, msgid, msgid);

pubcomp:
    /* create PUBCOMP packet */
    pb              = calloc(1, sizeof(CPB) + 4);
    if (!pb) goto quit;
    
    pb->bufsize     = 4;
    pb->buf[0]      = TYPE_PUBCOMP;
    pb->buf[1]      = 2;    /* size of msgid that follows */
    pos             = 2;

    util_put_ushort(msgid, &pb->buf[pos]);
    pos += 2;

    /* there is no response to a PUBCOMP so we discard 
     * the retry for this message id */
    mqtc_packet_discard(mqtc, msgid);
    
    /* put this packet on the send queue */
    rc = mqtc_session_send(mqtc, pb);

quit:
    // wtof("%s: exit rc=%d", __func__, rc);
    return rc;
}
