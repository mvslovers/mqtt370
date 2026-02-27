#include "mqtc370.h"

/* The logic here is to process a received packet as a MQTT client.
 * 
 * Some of the packet types should not occur as a MQTT client like
 * "CONNECT", "SUBSCRIBE", "UNSUBSCRIBE" and are simply dumped and
 * ignored from a processing perspective.
 */

/* process received packet */
int mqtc_session_process(MQTC *mqtc)
{
    int         rc          = 0;
    CPB         *pb;
    unsigned    type;

    pb = mqtc->rpb;
    if (!pb) goto quit;

    /* received packet callback */
    if (mqtc->f_rcpb) {
        /* we call the callback function using try() */
        try(mqtc->f_rcpb, mqtc, pb); 
    }

    /* isolate the request type and route to process handler */
    type = (pb->buf[0] & 0xF0);
    switch(type) {
    case TYPE_CONNECT:      // 0x10
        rc = mqtc_packet_dump(mqtc, pb, "CONNECT");
        break;
    case TYPE_CONNACK:      // 0x20
        rc = mqtc_process_connack(mqtc, pb);
        break;
    case TYPE_PUBLISH:      // 0x30 
        rc = mqtc_process_publish(mqtc, pb);
        break;
    case TYPE_PUBACK:       // 0x40
        rc = mqtc_process_puback(mqtc, pb);
        break;
    case TYPE_PUBREC:       // 0x50
        rc = mqtc_process_pubrec(mqtc, pb);
        break;
    case TYPE_PUBREL:       // 0x60 
        rc = mqtc_process_pubrel(mqtc, pb);
        break;
    case TYPE_PUBCOMP:      // 0x70
        rc = mqtc_process_pubcomp(mqtc, pb);
        break;
    case TYPE_SUBSCRIBE:    // 0x80
        rc = mqtc_packet_dump(mqtc, pb, "SUBSCRIBE");
        break;
    case TYPE_SUBACK:       // 0x90
        rc = mqtc_process_suback(mqtc, pb);
        break;
    case TYPE_UNSUBSCRIBE:  // 0xA0
        rc = mqtc_packet_dump(mqtc, pb, "UNSUBSCRIBE");
        break;
    case TYPE_UNSUBACK:     // 0xB0
        rc = mqtc_process_unsuback(mqtc, pb);
        break;
    case TYPE_PINGREQ:      // 0xC0
        rc = mqtc_packet_dump(mqtc, pb, "PINGREQ");
        break;
    case TYPE_PINGRESP:     // 0xD0
        rc = mqtc_process_pingresp(mqtc, pb);
        break;
    case TYPE_DISCONNECT:   // 0xE0
        rc = mqtc_packet_dump(mqtc, pb, "DISCONNECT");
        break;
    default:
        wtof("%s: *** Session Protocol Error ***", __func__);
        mqtc_packet_dump(mqtc, pb, "INVALID");
        rc = -1;
        break;
    }

quit:
    return rc;
}
