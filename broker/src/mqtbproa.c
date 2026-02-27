#include "mqtb370.h"

int mqtb_process_unsubscribe(MQTB *mqtb, MQTBS *mqtbs, PB *pb)
{
    int             rc      = 0;
    unsigned        len     = 0;
    unsigned        pos     = 0;
    MQSUB           *mqsub  = NULL;
    char            *topic  = NULL;
    unsigned short  msgid   = 0;
    int             valid;
    unsigned char   *uc;
    unsigned char   flags;
    unsigned char   dup;
    unsigned char   qos;

    if (!pb) goto quit;
    
    // mqtb_packet_dump(mqtb, mqtbs, pb, "UNSUBSCRIBE");

    /* get flags from fixed header */
    flags   = pb->buf[0] & 0x0F;
    dup     = flags & FIXED_FLAG_DUP ? 1 : 0;
    qos     = (flags & FIXED_FLAG_QOS) >> 1;

    /* decode variable header */
    if (!pb->varpos) {
        rc = util_length_decode(pb->buf, &len);
        if (!rc) goto quit;
        /* start of variable header */
        pb->varpos  = (unsigned short)rc;
    }
    pos = pb->varpos;
    
    /* get the message id */
    msgid = util_get_ushort(&pb->buf[pos]);
    pos += 2;

    if (mqtbs->proto == MQTBS_PROTO_500) {
        /* protocol version 5 can have properties */
        len = pb->buf[pos++];  /* length of properties */
        
        // wtodumpf(&pb->buf[pos], len, "%s: Properties", __func__);
        
        /* we're going to ignore the properties for now. */
        pos += len;
    }

do_payload:
    do {
        /* Get subscription length */
        len = util_get_ushort(&pb->buf[pos]);
        pos += 2;
        
        /* get subscription name in EBCDIC */
        topic = util_dup_a2e(&pb->buf[pos], len);
        pos += len;

        // wtodumpf(topic, len+1, "%s: topic", __func__);

        /* unsubscribe from topic */
        rc = mqtb_unsubscribe_topic(mqtb, mqtbs, topic);

        util_free_char(&topic);
    } while (pos < pb->bufsize);

    mqtb_reply_unsuback(mqtb, mqtbs, msgid);

quit:
    return rc;
}
