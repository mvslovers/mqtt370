#include "mqtb370.h"

int mqtb_process_disconnect(MQTB *mqtb, MQTBS *mqtbs, PB *pb)
{
    int             rc      = 0;
    unsigned char   flags;

    if (!pb) goto quit;
    
    /* get flags from fixed header */
    flags   = pb->buf[0] & 0x0F;

    /* no response is needed */

    /* set the session state to close */
    mqtbs->state = MQTBS_STATE_CLOSE;

quit:
    return rc;
}
