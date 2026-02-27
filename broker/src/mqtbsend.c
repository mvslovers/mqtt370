#include "mqtb370.h"

int mqtb_session_send(MQTB *mqtb, MQTBS *mqtbs, PB *pb)
{
    int         rc      = 0;
    int         lockrc;

    /* put packet buffer into session send queue */
    lockrc = lock(&mqtbs->spbq, LOCK_EXC);
    array_add(&mqtbs->spbq, pb);
    if (lockrc==0) unlock(&mqtbs->spbq, LOCK_EXC);

    /* wake up the wait thread to send the packet buffer */
    lockrc = lock(&mqtb->wait_ecb, LOCK_EXC);
    cthread_post(&mqtb->wait_ecb, 1);
    if (lockrc==0) unlock(&mqtb->wait_ecb, LOCK_EXC);
    
quit:
    return rc;

}
