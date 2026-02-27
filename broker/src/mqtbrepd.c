#include "mqtb370.h"

int mqtb_reply_pingresp(MQTB *mqtb, MQTBS *mqtbs)
{
    int             rc      = 0;
    unsigned        n;
    PB              *pb     = calloc(1, sizeof(PB) + 2);
    int             lockrc;

    if (!pb) goto quit;
    
    pb->bufsize     = 2;
    pb->buf[0]      = TYPE_PINGRESP;
    pb->buf[1]      = 0;    /* length of variable header */

    rc = mqtb_session_send(mqtb, mqtbs, pb);

quit:
    return rc;
}
