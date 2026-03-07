#include "mqtc370.h"

int mqtc_session_send(MQTC *mqtc, CPB *pb)
{
    int         rc      = 0;
    int         lockrc;

    /* put packet buffer into session send queue */
    lockrc = lock(&mqtc->spbq, LOCK_EXC);
    array_add(&mqtc->spbq, pb);
    if (lockrc==0) unlock(&mqtc->spbq, LOCK_EXC);

    /* wake up the task thread to send the packet buffer */
    lockrc = lock(&mqtc->task_ecb, LOCK_EXC);
    cthread_post(&mqtc->task_ecb, 1);
    if (lockrc==0) unlock(&mqtc->task_ecb, LOCK_EXC);
    
quit:
    return rc;

}
