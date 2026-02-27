#include "mqtb370.h"

int mqtb_reply_unsuback(MQTB *mqtb, MQTBS *mqtbs, unsigned msgid)
{
    int             rc      = 0;
    unsigned        n;
    PB              *pb     = calloc(1, sizeof(PB) + 4);
    unsigned char   *uc     = ((unsigned char *)&msgid) + 2;
    int             lockrc;
    unsigned        pos;

    if (!pb) goto quit;
    
    pb->bufsize     = 4;
    pb->buf[0]      = TYPE_UNSUBACK;
    pb->buf[1]      = 2;    /* length of variable header */
    /* PUBREL Variable Header */
    pb->buf[2]      = uc[0];    /* msgid MSB    */
    pb->buf[3]      = uc[1];    /* msgid LSB    */

    rc = mqtb_session_send(mqtb, mqtbs, pb);

quit:
    return rc;
}
