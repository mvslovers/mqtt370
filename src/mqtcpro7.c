#include "mqtc370.h"

/* mqtc_process_pubcomp()
 * Process PUBCOMP from client for QOS 2 PUBLISH message.
 */

int mqtc_process_pubcomp(MQTC *mqtc, CPB *pb)
{
    int             rc      = 4;
    unsigned        len     = 0;
    unsigned        pos     = 0;
    unsigned short  msgid;

    if (!pb) goto quit;
    
    // mqtc_packet_dump(mqtc, pb, "PUBCOMP");

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
    
    /* discard the retry packet for this message id */
    mqtc_packet_discard(mqtc, msgid);

    rc = 0;

quit:
    // wtof("%s: exit rc=%d", __func__, rc);
    return rc;
}
