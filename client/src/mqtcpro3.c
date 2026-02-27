#include "mqtc370.h"

int mqtc_process_publish(MQTC *mqtc, CPB *pb)
{
    int             rc      = 0;
    unsigned        len     = 0;
    unsigned        pos     = 0;
    char            *topic  = NULL;
    char            *msg    = NULL;
    unsigned        msg_len = 0;
    unsigned short  msgid   = 0;
    unsigned char   *uc;
    unsigned char   flags;
    unsigned char   dup;
    unsigned char   qos;
    unsigned char   retain;

    if (!pb) goto quit;
    
    // mqtc_packet_dump(mqtc, pb, "PUBLISH");

    /* get flags from fixed header */
    flags   = pb->buf[0] & 0x0F;
    dup     = flags & FIXED_FLAG_DUP ? 1 : 0;
    qos     = (flags & FIXED_FLAG_QOS) >> 1;
    retain  = flags & FIXED_FLAG_RETAIN ? 1 : 0;

    /* decode variable header */
    if (!pb->varpos) {
        rc = util_length_decode(pb->buf, &len);
        if (!rc) goto quit;
        /* start of variable header */
        pb->varpos  = (unsigned short)rc;
    }
    pos = pb->varpos;
    
    /* Get topic name length */
    len = util_get_ushort(&pb->buf[pos]);
    pos += 2;

    /* Get the topic name as a string */
    topic = util_dup_a2e(&pb->buf[pos], len);
    pos += len;

    if (qos > 0) {
        /* get the message id */
        msgid = util_get_ushort(&pb->buf[pos]);
        pos += 2;
    }
    
    // wtof("%s: qos=%u msgid=0x%04X topic=\"%s\"", __func__, qos, msgid, topic);

#if 0   /* not needed */
    if (!(mqtb_topic_is_valid(topic))) {
        /* invalid topic name */
        wtof("MQTTD300W Invalid topic=\"%s\"", topic);
        goto doreply;
    }
    
    if (mqtbs->proto == MQTBS_PROTO_500) {
        /* protocol version 5 can have properties */
        len = pb->buf[pos++];  /* length of properties */
        
        // wtodumpf(&pb->buf[pos], len, "%s: Properties", __func__);
        
        /* we're going to ignore the properties for now. */
        pos += len;
    }
#endif

dopayload:
    len = pb->bufsize - pos;
    if (len) {
        /* we have a message in the payload */
        msg = util_memdup(&pb->buf[pos], len);
        msg_len = len;
        pos += len;
    }

    /* do callback processing */
    if (mqtc->f_rtopic_bin) {
        /* callback for published topic with binary message */
        try(mqtc->f_rtopic_bin, mqtc, topic, msg, msg_len);
    }
    
    if (mqtc->f_rtopic) {
        if (msg) {
            uc = (unsigned char*)msg;
            /* convert ASCII to EBCDIC */
            for(pos=0; pos < msg_len; pos++) {
                uc[pos] = (unsigned char) utila2e[uc[pos]];
            }
        }
        /* callback for published topic with ASCII --> EBCDIC message */
        try(mqtc->f_rtopic, mqtc, topic, msg, msg_len);
    }

doreply:
    // wtof("%s: doreply: qos=%u", __func__, qos);
    if (qos==1) {
        /* send PUBACK */
        // wtof("%s: calling mqtc_reply_puback()", __func__);
        mqtc_reply_puback(mqtc, msgid);
    }
    else if (qos==2) {
        /* send PUBREC */
        // wtof("%s: calling mqtc_reply_pubrec()", __func__);
        mqtc_reply_pubrec(mqtc, msgid);
    }

quit:
    util_free_char(&msg);
    util_free_char(&topic);
    return rc;
}
