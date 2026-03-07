#include "mqtc370.h"

int mqtc_reply_puback(MQTC *mqtc, unsigned msgid)
{
    int             rc      = 8;
    CPB             *pb     = calloc(1, sizeof(CPB) + 4);
    unsigned char   *uc     = ((unsigned char *)&msgid) + 2;
    int             lockrc;
    
    if (!pb) goto quit;
    
    pb->bufsize     = 4;
    pb->buf[0]      = TYPE_PUBACK;
    pb->buf[1]      = 2;    /* length of variable header */
    /* PUBACK Variable Header */
    pb->buf[2]      = uc[0];    /* msgid MSB    */
    pb->buf[3]      = uc[1];    /* msgid LSB    */

    rc = mqtc_session_send(mqtc, pb);

quit:
    return rc;
}
