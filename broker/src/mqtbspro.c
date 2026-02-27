#include "mqtb370.h"

static int session_proc(MQTB *mqtb, MQTBS *mqtbs);
static const char *session_state(MQTBS *mqtbs);

int mqtb_session_process(MQTB *mqtb, MQTBS *mqtbs)
{
    int         rc      = 0;
    PB          *pb     = mqtbs->rpb;

    if (pb->pos == pb->bufsize) {
        /* full packet buffer read */
        // wtodumpf(pb->buf, pb->bufsize, "%s: Full Packet Buffer", __func__);

        rc = session_proc(mqtb, mqtbs);

        if (mqtbs->rpb) {
            free(mqtbs->rpb);
            mqtbs->rpb = NULL;
        }
    }

    return rc;
}

static const char *session_state(MQTBS *mqtbs)
{
    switch(mqtbs->state) {
    case MQTBS_STATE_INIT: return "INIT";
    case MQTBS_STATE_WAIT: return "WAIT";
    case MQTBS_STATE_PROC: return "PROC";
    case MQTBS_STATE_READ: return "READ";
    case MQTBS_STATE_CLOSE: return "CLOSE";
    case MQTBS_STATE_SHUTDOWN: return "SHUTDOWN";
    }
    
    return "INVALID";
}

static int session_proc(MQTB *mqtb, MQTBS *mqtbs)
{
    int         rc      = 0;
    PB          *pb;
    unsigned    type;

    pb = mqtbs->rpb;
    if (!pb) goto quit;
    
    // wtodumpf(pb, sizeof(PB), "%s: PB", __func__);
    
    /* isolate the request type and route to session handler */
    type = (pb->buf[0] & 0xF0);
    switch(type) {
    case TYPE_CONNECT:      // 0x10
        rc = mqtb_process_connect(mqtb, mqtbs, pb);
        break;
    case TYPE_CONNACK:      // 0x20 - broker should not get this
        rc = mqtb_packet_dump(mqtb, mqtbs, pb, "CONNACK");
        break;
    case TYPE_PUBLISH:      // 0x30 
        rc = mqtb_process_publish(mqtb, mqtbs, pb);
        break;
    case TYPE_PUBACK:       // 0x40
        rc = mqtb_process_puback(mqtb, mqtbs, pb);
        break;
    case TYPE_PUBREC:       // 0x50
        rc = mqtb_process_pubrec(mqtb, mqtbs, pb);
        break;
    case TYPE_PUBREL:       // 0x60 
        rc = mqtb_process_pubrel(mqtb, mqtbs, pb);
        break;
    case TYPE_PUBCOMP:      // 0x70
        rc = mqtb_process_pubcomp(mqtb, mqtbs, pb);
        break;
    case TYPE_SUBSCRIBE:    // 0x80 
        rc = mqtb_process_subscribe(mqtb, mqtbs, pb);
        break;
    case TYPE_SUBACK:       // 0x90 - broker should not get this
        rc = mqtb_packet_dump(mqtb, mqtbs, pb, "SUBACK");
        break;
    case TYPE_UNSUBSCRIBE:  // 0xA0
        rc = mqtb_process_unsubscribe(mqtb, mqtbs, pb);
        break;
    case TYPE_UNSUBACK:     // 0xB0 - broker should not get this
        rc = mqtb_packet_dump(mqtb, mqtbs, pb, "UNSUBACK");
        break;
    case TYPE_PINGREQ:      // 0xC0
        rc = mqtb_process_pingreq(mqtb, mqtbs, pb);
        break;
    case TYPE_PINGRESP:     // 0xD0
        rc = mqtb_packet_dump(mqtb, mqtbs, pb, "PINGRESP");
        break;
    case TYPE_DISCONNECT:   // 0xE0
        rc = mqtb_process_disconnect(mqtb, mqtbs, pb);
        break;
    default:
        wtof("%s: *** Session Protocol Error ***", __func__);
        mqtb_packet_dump(mqtb, mqtbs, pb, "INVALID");
        rc = -1;
        break;
    }

quit:
    return rc;
}
