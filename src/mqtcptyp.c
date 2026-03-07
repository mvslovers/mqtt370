#include "mqtc370.h"

const char *mqtc_packet_type(CPB *pb)
{
    const char  *type = "UNKNOWN";
    
    switch((pb->buf[0] & 0xF0)) {
    case TYPE_CONNECT:      type = "CONNECT";       break;
    case TYPE_CONNACK:      type = "CONNACK";       break;
    case TYPE_PUBLISH:      type = "PUBLISH";       break;
    case TYPE_PUBACK:       type = "PUBACK";        break;
    case TYPE_PUBREC:       type = "PUBREC";        break;
    case TYPE_PUBREL:       type = "PUBREL";        break;
    case TYPE_PUBCOMP:      type = "PUBCOMP";       break;
    case TYPE_SUBSCRIBE:    type = "SUBSCRIBE";     break;
    case TYPE_SUBACK:       type = "SUBACK";        break;
    case TYPE_UNSUBSCRIBE:  type = "UNSUBSCRIBE";   break;
    case TYPE_UNSUBACK:     type = "UNSUBACK";      break;
    case TYPE_PINGREQ:      type = "PINGREQ";       break;
    case TYPE_PINGRESP:     type = "PINGRESP";      break;
    case TYPE_DISCONNECT:   type = "DISCONNECT";    break;
    }

    return type;
}
