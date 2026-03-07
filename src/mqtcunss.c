#include "mqtc370.h"

int mqtc_unsub(MQTC *mqtc, const char *topic)
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
    
    /* check for socket processing thread */
    rc += 4;
    if (!mqtc->task) goto quit;

    topiclen = strlen(topic);

    /* calculate size of area after the fixed header */
    remaining = sizeof(msgid) + 2 + topiclen;

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

    /* construct SUBSCRIBE packet */
    pos = 0;
    pb->buf[pos++] = TYPE_UNSUBSCRIBE | 0x02; /* QOS 1 */

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
    
    /* message/packet id */
    do {
        msgid = ++mqtc->packetid;    /* util_prng(); */
    } while(msgid==0);
    util_put_ushort(msgid, &pb->buf[pos]);
    pos += 2;
    
    /* payload portion starts here */
    pb->paypos = pos;

    /* topic name */
    len = topiclen;
    util_put_ushort(len, &pb->buf[pos]);
    pos += 2;
    for(i=0; i < len; i++) {
        pb->buf[pos+i] = utile2a[topic[i]];
    }
    pos += len;
    
    // wtodumpf(pb->buf, pb->bufsize, "%s: PB", __func__);
    
    /* add this packet to the retry queue */
    mqtc_packet_retry(mqtc, msgid, pb);
    
    /* put SUBSCRIBE packet on send queue */
    rc = mqtc_session_send(mqtc, pb);

quit:
    return rc;
}
