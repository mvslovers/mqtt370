#include "mqtb370.h"

int mqtb_process_subscribe(MQTB *mqtb, MQTBS *mqtbs, PB *pb)
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
    unsigned char   sqos;
    unsigned        *aqos   = NULL;

    if (!pb) goto quit;
    
    // mqtb_packet_dump(mqtb, mqtbs, pb, "SUBSCRIBE");

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

        sqos = pb->buf[pos++];

        // wtof("%s: msgid=%u topic=\"%s\" qos=%u", __func__, msgid, topic, sqos);
        valid = mqtb_topic_sub_valid(topic);
        // wtof("%s: %s subscription topic", __func__, valid ? "VALID" : "INVALID");
        
        if (valid) {
            /* subscribe to topic */
            rc = mqtb_subscribe_topic(mqtb, mqtbs, topic, &sqos);
            if (rc) sqos |= 0x80;   /* indicate failure */
        }
        else {
            /* invalid topic filter (subscription) */
            sqos |= 0x80;
        }
        array_add(&aqos, (void*)((unsigned)sqos));

        util_free_char(&topic);
    } while (pos < pb->bufsize);

    mqtb_reply_suback(mqtb, mqtbs, msgid, aqos);

quit:
    if (aqos) array_free(&aqos);
    return rc;
}
