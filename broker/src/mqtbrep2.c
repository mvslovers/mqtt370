#include "mqtb370.h"

int mqtb_reply_connack(MQTB *mqtb, MQTBS *mqtbs, unsigned rsn)
{
    int     rc      = 8;
    PB      *pb     = calloc(1, sizeof(PB) + 4);
    int     lockrc;
    
    if (!pb) goto quit;
    
    pb->bufsize     = 4;
    pb->buf[0]      = TYPE_CONNACK;
    pb->buf[1]      = 2;    /* size of variable header that follows */
    /* CONNACK Variable Header */
    pb->buf[2]      = 0;                    /* reserved */
    pb->buf[3]      = (unsigned char)rsn;   /* reason code */

    rc = mqtb_session_send(mqtb, mqtbs, pb);

quit:
    return rc;
}
