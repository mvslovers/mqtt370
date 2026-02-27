#include "mqtb370.h"

/* mqtb_process_pubrel()
 * Process PUBREL from client for QOS 2 PUBLISH message.
 */

int mqtb_process_pubrel(MQTB *mqtb, MQTBS *mqtbs, PB *pb)
{
    int             rc      = 0;
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

    rc = 8;
    
    /* Get msgid */
    msgid = util_get_ushort(&pb->buf[pos]);
    pos += 2;

    // wtof("%s: msgid=%04X (%u)", __func__, msgid, msgid);
    
    /* Are we waiting for a response? */
    if (!(mqtbs->flags & MQTBS_FLAG_WAITRESP)) {
        /* no, we're not waiting for a response */
        // wtof("%s: session is NOT waiting", __func__);
        goto pubcomp;
    }
    
    /* make sure we have a send packet to clear */
    pb = mqtbs->spb;
    if (!pb) goto pubcomp;
    if (pb->pos != pb->bufsize) goto pubcomp;

    /* clear the current send packet */
    free(pb);
    mqtbs->spb = NULL;
    // wtof("%s: send packet deleted", __func__);

pubcomp:
    /* create PUBCOMP packet */
    pb              = calloc(1, sizeof(PB) + 4);
    if (!pb) goto quit;
    
    pb->bufsize     = 4;
    pb->buf[0]      = TYPE_PUBCOMP;
    pb->buf[1]      = 2;    /* size of msgid that follows */
    pos             = 2;

    util_put_ushort(msgid, &pb->buf[pos]);
    pos += 2;

    if (mqtbs->flags & MQTBS_FLAG_WAITRESP) {
        /* put PUBREL packet as current packet to send */
        mqtbs->spb      = pb;
        // wtof("%s: PUBCOMP packet ready to send", __func__);
    }
    else {
        // wtof("%s: PUBCOMP packet queued for send", __func__);
        mqtb_session_send(mqtb, mqtbs, pb);
    }

waitresp:
    if (mqtbs->flags & MQTBS_FLAG_WAITRESP) {
        /* clear the wait response flag */
        mqtbs->flags &= ~MQTBS_FLAG_WAITRESP;
        // wtof("%s: wait response flag cleared", __func__);
    }
    rc = 0;

quit:
    // wtof("%s: exit rc=%d", __func__, rc);
    return rc;
}
