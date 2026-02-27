#include "mqtb370.h"

int mqtb_reply_suback(MQTB *mqtb, MQTBS *mqtbs, unsigned msgid, unsigned *aqos)
{
    int             rc      = 0;
    unsigned        n;
    unsigned        count   = array_count(&aqos);
    PB              *pb     = calloc(1, sizeof(PB) + count + 4);
    unsigned char   *uc     = ((unsigned char *)&msgid) + 2;
    int             lockrc;
    unsigned        pos;

    if (!pb) goto quit;
    
    pb->bufsize     = 4;
    pb->buf[0]      = TYPE_SUBACK;
    pb->buf[1]      = 2;    /* length of variable header */
    /* PUBREL Variable Header */
    pb->buf[2]      = uc[0];    /* msgid MSB    */
    pb->buf[3]      = uc[1];    /* msgid LSB    */

    for(pos=4, n=0; n < count; n++) {
        pb->buf[pos++] = (unsigned char)(aqos[n]);
    }
    
    pb->bufsize = pos;
    pb->buf[1]  += (unsigned char)n;

    rc = mqtb_session_send(mqtb, mqtbs, pb);

quit:
    return rc;
}
