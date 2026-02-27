#include "mqtb370.h"

int mqtb_process_publish(MQTB *mqtb, MQTBS *mqtbs, PB *pb)
{
    int             rc      = 0;
    unsigned        len     = 0;
    unsigned        pos     = 0;
    MQPUB           *mqpub  = NULL;
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
    
    // mqtb_packet_dump(mqtb, mqtbs, pb, "PUBLISH");

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

dopayload:
    len = pb->bufsize - pos;
    if (len) {
        /* we have a message in the payload */
        msg = util_memdup(&pb->buf[pos], len);
        msg_len = len;
        pos += len;
    }

    // if (topic) wtof("%s: Topic=\"%s\" msgid=%u dup=%u qos=%u retain=%u", 
    //     __func__, topic, msgid, dup, qos, retain);
    // if (msg && msg_len) {
    //     wtodumpf(msg, msg_len, "%s: Message", __func__);
    // }

    /* Publish the topic and message */
    mqpub = calloc(1, sizeof(MQPUB));
    if (!mqpub) goto quit;

    strcpy(mqpub->eye, MQPUB_EYE);
    mqpub->retain   = retain;
    mqpub->qos      = qos;
    mqpub->msgid    = msgid;
    mqpub->dup      = dup;
    mqpub->topic    = topic;
    mqpub->msg      = msg;
    mqpub->msg_len  = msg_len;
    
    topic = NULL;
    msg   = NULL;

    // cthread_queue_add(mqtb->mgr, mqpub);
    mqtb_publish_request(mqtb, mqpub);

doreply:
    if (qos==1) {
        /* send PUBACK */
        mqtb_reply_puback(mqtb, mqtbs, msgid);
    }
    else if (qos==2) {
        /* send PUBREC */
        mqtb_reply_pubrec(mqtb, mqtbs, msgid);
    }

quit:
    util_free_char(&msg);
    util_free_char(&topic);
    return rc;
}
