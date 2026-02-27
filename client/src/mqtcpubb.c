#include "mqtc370.h"

int mqtc_pub_bin(MQTC *mqtc, unsigned qos, unsigned retain, const char *topic, const void *msg, unsigned msglen)
{
    int             rc      = 4;
    unsigned        topiclen;
    unsigned        remaining;
    unsigned        bytes;
    unsigned        packetlen;
    unsigned        len;
    unsigned        pos;
    unsigned        i;
    CPB             *pb;
    unsigned short  msgid;
    unsigned char   *flags;
    
    /* check for client handle */
    if (!mqtc) goto quit;
    if (!topic) goto quit;

    /* check for open socket to broker */
    rc += 4;
    if (mqtc->sock < 0) goto quit;
    
#if 0
    /* check for socket processing thread */
    rc += 4;
    if (!mqtc->task) goto quit;
#endif

    /* check for topic */
    rc += 4;
    if (!topic) goto quit;
    if (!*topic) goto quit;

    /* validate topic */
    if (!util_topic_is_valid(topic)) goto quit;
    
    /* get topic length */
    topiclen = strlen(topic);

    /* sanity check */
    if (qos > 2) qos = 2;
    if (retain > 1) retain = 1;

    /* calculate size of area after the fixed header */
    remaining = 2 + topiclen + msglen;
    if (qos) remaining += 2;    /* msgid */

    /* calulate the packed size of the remaining value in the fixed header */
    bytes = 1;
    if (remaining > 127) bytes++;
    if (remaining > 16383) bytes++;
    if (remaining > 2097151) bytes++;

    /* calculate the size needed for the packet buffer */
    packetlen = 1 + bytes + remaining;

    /* allocate a packet buffer */
    rc += 4;
    pb = calloc(1, sizeof(CPB) + packetlen);
    if (!pb) goto quit;

    /* construct PUBLISH packet */
    pos = 0;
    pb->buf[pos++] = TYPE_PUBLISH | ((qos << 1)|retain);
    /* Note: we're not using QOS 1 on the subscribe itself because
     * we're limiting this API to requesting one subscribe topic
     * per request. Therefore QOS 0 is being used.
     */

    /* encode the "remaining" value into the packet buffer */
    if (bytes != (len=util_length_encode(remaining, &pb->buf[pos]))) {
        wtof("%s: error encoding packet remaining value=%u bytes=%u len=%d", 
            __func__, remaining, bytes, len);
        free(pb);
        goto quit;
    }

    pb->bufsize = packetlen;
    pos         += bytes;
    
    /* variable portion starts here */
    pb->varpos  = pos;

    /* topic */
    len = topiclen;
    util_put_ushort(len, &pb->buf[pos]);
    pos += 2;
    for(i=0; i < len; i++) {
        pb->buf[pos+i] = utile2a[topic[i]];
    }
    pos += len;

    if (qos) {
        /* message id */
        do {
            msgid = ++mqtc->packetid;    /* util_prng(); */
        } while(msgid==0);
        util_put_ushort(msgid, &pb->buf[pos]);
        pos += 2;
    }
    
    /* payload portion starts here */
    pb->paypos = pos;

    /* message */
    if (msg && msglen) {
        memcpy(&pb->buf[pos], msg, msglen);
        pos += len;
    }

    // wtodumpf(pb->buf, pb->bufsize, "%s: PB", __func__);

    if (qos) {
        /* save this packet for retry */
        mqtc_packet_retry(mqtc, msgid, pb);
    }
    
    /* put PUBLISH packet on send queue */
    rc = mqtc_session_send(mqtc, pb);

quit:
    return rc;
}
