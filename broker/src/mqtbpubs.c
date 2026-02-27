#include "mqtb370.h"

int mqtb_publish_subscriber(MQTB *mqtb, MQPUB *mqpub, unsigned char subqos, MQTBS *mqtbs)
{
    int         rc      = 0;
    int         lockrc  = 8;
    PB          *pb;
    unsigned    topiclen;
    unsigned    remaining;
    unsigned    bytes;
    unsigned    packetlen;
    unsigned    pos;
    unsigned char *uc;
    unsigned char qos;
    unsigned short msgid = 0;
    unsigned char  flags;
    
    // wtof("%s: session %p", __func__, mqtbs);
    // wtodumpf(mqtbs, sizeof(MQTBS), "%s: MQTBS", __func__);

    /* don't publish to sessions that are about to be closed */
    if (mqtbs->state == MQTBS_STATE_CLOSE) goto quit;

    /* select qos level for this publish to subscriber request */
    qos = subqos;   /* start with the subscribers qos */
    if (mqpub->qos < subqos) qos = mqpub->qos; /* downgrade to published qos */
    if (qos > 2) qos = 2;   /* sanity check */

    /* set flags for publish */
    flags   = (qos << 1);
    if (mqpub->retain) flags |= 1;

    /* calculate size of area after the fixed header */
    topiclen  = strlen(mqpub->topic);
    remaining = sizeof(unsigned short) +    /* topic length     */
                topiclen               +    /* topic string     */
                mqpub->msg_len;             /* message length   */

    if (qos) {
        remaining += sizeof(unsigned short);/* size of msgid    */
    }
             
    if (mqtbs->proto == MQTBS_PROTO_500) {
        remaining += 1; /* properties length field (none in this release) */
    }

    /* calulate the packed size of the remaining value in the fixed header */
    bytes = 1;
    if (remaining > 127) bytes++;
    if (remaining > 16383) bytes++;
    if (remaining > 2097151) bytes++;

    /* calculate the size needed for the packet buffer */
    packetlen = 1 + bytes + remaining;

    // wtof("%s: bytes=%u remaining=%u packetlen=%u", __func__, bytes, remaining, packetlen);

    /* allocate a packet buffer */
    pb = calloc(1, sizeof(PB) + packetlen);
    if (!pb) {
        wtof("%s: Out Of Memory. Packet length=%u", __func__, sizeof(PB) + packetlen);
        goto quit;
    }

    pos = 0;
    pb->bufsize     = packetlen;
    pb->buf[pos++]  = TYPE_PUBLISH | flags;

    // wtof("%s: 0 pos=%u", __func__, pos);
    // wtodumpf(pb->buf, pb->bufsize, "%s: Packet Buffer", __func__);

    // wtof("%s: encoding remaining", __func__);

    /* encode the "remaining" value into the packet buffer */
    if (bytes != (rc=util_length_encode(remaining, &pb->buf[pos]))) {
        wtof("%s: error encoding packet remaining value=%u bytes=%u rc=%d", 
            __func__, remaining, bytes, rc);
        free(pb);
        goto quit;
    }
    pos             += bytes;

    // wtof("%s: 1 pos=%u", __func__, pos);
    // wtodumpf(pb->buf, pb->bufsize, "%s: Packet Buffer", __func__);

    /* put the topic length into the packet buffer */
    util_put_ushort(topiclen, &pb->buf[pos]);
    pos += 2;

    // wtof("%s: 2 pos=%u", __func__, pos);
    // wtodumpf(pb->buf, pb->bufsize, "%s: Packet Buffer", __func__);

    /* put the topic name into the packet buffer */
    for(bytes=0; bytes < topiclen; bytes++) {
        pb->buf[pos++] = utile2a[mqpub->topic[bytes]];
    }

    // wtof("%s: 3 pos=%u", __func__, pos);
    // wtodumpf(pb->buf, pb->bufsize, "%s: Packet Buffer", __func__);

    if (qos) {
        /* put the msgid into the packet bbuffer */
        msgid = util_prng();
        util_put_ushort(msgid, &pb->buf[pos]);
        pos += 2;
    }

    // wtof("%s: 4 pos=%u", __func__, pos);
    // wtodumpf(pb->buf, pb->bufsize, "%s: Packet Buffer", __func__);

    /* put the message data (if any) into the packet buffer */
    if (mqpub->msg && mqpub->msg_len) {
        memcpy(&pb->buf[pos], mqpub->msg, mqpub->msg_len);
        pos += mqpub->msg_len;
    }

    /* put the packet buffer into the session send queue */
    rc = mqtb_session_send(mqtb, mqtbs, pb);
    
quit:
    return rc;
}
