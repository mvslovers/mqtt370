#include "mqtc370.h"

static const char   protocol_name[] = "MQTT";

int mqtc_connect(MQTC *mqtc)
{
    int         rc      = 4;
    unsigned    userlen = mqtc->userid ? strlen(mqtc->userid) : 0;
    unsigned    passlen = mqtc->password ? strlen(mqtc->password) : 0;
    unsigned    protlen = 4;   /* strlen(protocol_name); */
    unsigned    clielen = 0;
    unsigned    remaining;
    unsigned    bytes;
    unsigned    packetlen;
    unsigned    len;
    unsigned    pos;
    unsigned    i;
    CPB         *pb;
    unsigned char *flags;
    char        client_id[24];
    
    /* check for client handle */
    if (!mqtc) goto quit;

    /* check for open socket to broker */
    rc += 4;
    if (mqtc->sock < 0) goto quit;
    
#if 0
    /* check for socket processing thread */
    rc += 4;
    if (!mqtc->task) goto quit;
#endif

    /* calculate size of packet for connect */
    if (!mqtc->clientid) {
        sprintf(client_id, "mqtt370_0x%08X", util_prng());
        mqtc->clientid = strdup(client_id);
    }
    clielen = strlen(mqtc->clientid);

    /* calculate size of area after the fixed header */
    remaining = 2 + protlen + 1 + 1 + 2 + 2 + clielen;

    if (mqtc->will_topic) {
        remaining += 2 + strlen(mqtc->will_topic) + 2 + mqtc->will_len;
    }

    if (mqtc->userid)   remaining += 2 + userlen;
    if (mqtc->password) remaining += 2 + passlen;

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

    /* construct CONNECT packet */
    pos = 0;
    pb->buf[pos++] = TYPE_CONNECT;

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
    
    /* client id */
    len = protlen;
    util_put_ushort(len, &pb->buf[pos]);
    pos += 2;
    for(i=0; i < len; i++) {
        pb->buf[pos+i] = utile2a[protocol_name[i]];
    }
    pos += len;
    
    /* protocol version */
    pb->buf[pos++] = 4;     /* 3.1.1 protocol */
    
    /* flags */
    flags = &pb->buf[pos++];
    if (mqtc->userid)       *flags |= 0x80; /* 1... .... userid     */
    if (mqtc->password)     *flags |= 0x40; /* .1.. .... passowrd   */
    if (mqtc->will_retain)  *flags |= 0x20; /* ..1. .... Will RETAIN*/
    *flags |= (mqtc->will_qos & 0x03) << 3; /* ...1 1... Will QOS   */
    if (mqtc->will_topic)   *flags |= 0x04; /* .... .1.. Will       */
    if (mqtc->clean_session)*flags |= 0x02; /* .... ..1. clean      */
                                    /* 0x01 == .... ...1 not used   */
    /* keep alive timer */
    util_put_ushort(mqtc->keepalive, &pb->buf[pos]);
    pos += 2;
    
    /* payload portion starts here */
    pb->paypos  = pos;

    /* client id */
    len = clielen;
    util_put_ushort(len, &pb->buf[pos]);
    pos += 2;
    for(i=0; i < len; i++) {
        pb->buf[pos+i] = utile2a[mqtc->clientid[i]];
    }
    pos += len;

    /* will topic and message */
    if (mqtc->will_topic) {
        len = strlen(mqtc->will_topic);
        util_put_ushort(len, &pb->buf[pos]);
        pos += 2;
        for(i=0; i < len; i++) {
            pb->buf[pos+i] = utile2a[mqtc->will_topic[i]];
        }
        pos += len;
        
        len = mqtc->will_len;
        util_put_ushort(len, &pb->buf[pos]);
        pos += 2;
        if (mqtc->will_msg) memcpy(&pb->buf[pos], mqtc->will_msg, len);
        pos += len;
    }

    /* userid */
    if (userlen) {
        len = userlen;
        util_put_ushort(len, &pb->buf[pos]);
        pos += 2;
        for(i=0; i < len; i++) {
            pb->buf[pos+i] = utile2a[mqtc->userid[i]];
        }
        pos += len;
    }

    /* password */
    if (passlen) {
        len = passlen;
        util_put_ushort(len, &pb->buf[pos]);
        pos += 2;
        for(i=0; i < len; i++) {
            pb->buf[pos+i] = utile2a[mqtc->password[i]];
        }
        pos += len;
    }

    // wtodumpf(pb->buf, pb->bufsize, "%s: PB", __func__);
    
    /* put CONNECT packet on send queue */
    rc = mqtc_session_send(mqtc, pb);

quit:
    return rc;
}
