#include "mqtb370.h"

/* mqtb_process_pubrec()
 * Process PUBREC from client for QOS 2 PUBLISH message.
 */

int mqtb_process_pubrec(MQTB *mqtb, MQTBS *mqtbs, PB *pb)
{
    int             rc      = 0;
    unsigned        len     = 0;
    unsigned        pos     = 0;
    unsigned short  msgid;

    if (!pb) goto quit;
    
    // mqtb_packet_dump(mqtb, mqtbs, pb, "PUBREC");

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
        wtof("%s: session is NOT waiting", __func__);
        goto quit;
    }
    
    /* make sure we have a send packet to clear */
    pb = mqtbs->spb;
    if (!pb) goto waitresp;
    if (pb->pos != pb->bufsize) goto waitresp;

    /* clear the current send packet */
    free(pb);
    mqtbs->spb = NULL;
    // wtof("%s: send packet deleted", __func__);

    /* create PUBREL packet */
    pb              = calloc(1, sizeof(PB) + 4);
    if (!pb) goto quit;
    
    pb->bufsize     = 4;
    pb->buf[0]      = TYPE_PUBREL | 0x02; /* QOS 1 flag set */
    pb->buf[1]      = 2;    /* size of msgid that follows */
    pos             = 2;

    util_put_ushort(msgid, &pb->buf[pos]);
    pos += 2;

    /* put PUBREL packet as current packet to send */
    mqtbs->spb      = pb;
    // wtof("%s: PUBREL packet ready to send", __func__);

waitresp:
    /* clear the wait response flag */
    mqtbs->flags &= ~MQTBS_FLAG_WAITRESP;
    // wtof("%s: wait response flag cleared", __func__);
    rc = 0;

quit:
    // wtof("%s: exit rc=%d", __func__, rc);
    return rc;
}
