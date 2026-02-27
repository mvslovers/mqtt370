#include "mqtb370.h"

static int remove_session(MQSUB *mqsub, MQTBS *mqtbs);

int mqtb_unsubscribe_session(MQTB *mqtb, MQTBS *mqtbs)
{
    int             rc      = 0;
    int             lockrc;
    MQSUB           *mqsub;
    unsigned        n;
    unsigned        count;
    
    lockrc = lock(&mqtb->subscribers, LOCK_EXC);

    count = array_count(&mqtb->subscribers);
    for(n=count; n > 0; n--) {
        mqsub = array_get(&mqtb->subscribers, n);

        if (!mqsub) continue;

        /* remove this session from the subcriber record */
        remove_session(mqsub, mqtbs);

        /* check for subscription with no subscribers */
        if (array_count(&mqsub->mqtbs)==0) {
            /* remove the subscriber record from the subscribers array */
            array_del(&mqtb->subscribers, n);
            /* delete the subscriber record */
            util_free_char(&mqsub->topic);
            array_free(&mqsub->mqtbs);
            free(mqsub);
        }
    }

quit:
    if (lockrc==0) unlock(&mqtb->subscribers, LOCK_EXC);
    return rc;
}

static int remove_session(MQSUB *mqsub, MQTBS *mqtbs)
{
    unsigned    n, count;
    
    count = array_count(&mqsub->mqtbs);
    for(n=count; n > 0; n--) {
        MQTBS *s = array_get(&mqsub->mqtbs, n);
        
        if (!s) continue;
        
        if (s == mqtbs) {
            array_del(&mqsub->mqtbs, n);
        }
    }
    
    return 0;
}
