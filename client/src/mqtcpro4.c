#include "mqtc370.h"

/* mqtc_process_puback()
 * Process PUBACK from client for QOS 1 PUBLISH message.
 */

int mqtc_process_puback(MQTC *mqtc, CPB *pb)
{
    int             rc      = 4;
    unsigned        len     = 0;
    unsigned        pos     = 0;
    unsigned short  msgid;

    if (!pb) goto quit;
    
    // mqtc_packet_dump(mqtc, pb, "PUBACK");

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

#if 0
    /* We're not doing WAITRESP processing. */
    
    /* Are we waiting for a response? */
    if (!(mqtc->flags & MQTC_FLAG_WAITRESP)) {
        /* no, we're not waiting for a response */
        wtof("%s: session is NOT waiting", __func__);
        goto quit;
    }
    
    /* make sure we have a send packet to clear */
    pb = mqtc->spb;
    if (!pb) goto waitresp;
    if (pb->pos != pb->bufsize) goto waitresp;

    /* clear the current send packet */
    free(pb);
    mqtc->spb = NULL;
    // wtof("%s: send packet deleted", __func__);

waitresp:
    /* clear the wait response flag */
    mqtc->flags &= ~MQTC_FLAG_WAITRESP;
    // wtof("%s: wait response flag cleared", __func__);
    rc = 0;
#endif

    /* remove the retry packet for this message id */
    mqtc_packet_discard(mqtc, msgid);

quit:
    // wtof("%s: exit rc=%d", __func__, rc);
    return rc;
}
