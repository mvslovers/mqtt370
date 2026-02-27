#include "mqtb370.h"

int mqtb_process_pingreq(MQTB *mqtb, MQTBS *mqtbs, PB *pb)
{
    int             rc      = 0;
    unsigned        len     = 0;
    unsigned        pos     = 0;
    unsigned char   flags;

    if (!pb) goto quit;
    
    // mqtb_packet_dump(mqtb, mqtbs, pb, "PINGREQ");

    /* get flags from fixed header */
    flags   = pb->buf[0] & 0x0F;

    rc = mqtb_reply_pingresp(mqtb, mqtbs);

quit:
    return rc;
}
