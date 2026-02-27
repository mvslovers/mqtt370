#include "mqtc370.h"

int mqtc_send_ping(MQTC *mqtc)
{
    int             rc      = 0;
    unsigned        n;
    CPB             *cpb    = calloc(1, sizeof(CPB) + 2);
    int             lockrc;

    if (!cpb) goto quit;
    
    cpb->bufsize    = 2;
    cpb->buf[0]     = TYPE_PINGREQ;
    cpb->buf[1]     = 0;    /* length of variable header */

    rc = mqtc_session_send(mqtc, cpb);

quit:
    return rc;
}
