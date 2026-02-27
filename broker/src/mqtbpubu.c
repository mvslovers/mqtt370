#include "mqtb370.h"

int mqtb_publish_update(MQTB *mqtb, MQPUB *mqpub)
{
    int         rc      = 0;
    int         lockrc;
    unsigned    n;
    unsigned    count;
    time64_t    now;

    if (__64_is_zero(&mqpub->expires)) {
        /* set the time this topic will expire */
        time64(&now);
        __64_add_u32(&now, mqtb->mqtbc->topic_expire, &mqpub->expires);
    }

    if (mqpub->msg_len==0) {
        /* zero length messages can not be retained */
        mqpub->retain = 0;
    }

    lockrc = lock(&mqtb->topics, LOCK_EXC);
    count = array_count(&mqtb->topics);
    for(n=0; n < count; n++) {
        MQPUB *p = mqtb->topics[n];
        
        if (!p) continue;
        if (strcmp(p->topic, mqpub->topic)==0) {
            if (mqpub->retain) {
                /* replace the old topic with the new topic */
                mqtb->topics[n] = mqpub;
            }
            else {
                /* delete old topic from array */
                array_del(&mqtb->topics, n+1);
            }
            /* delete old topic */
            mqtb_publish_free(mqtb, &p);
            goto quit;
        }
    }
    
    /* must be a new topic */
    if (mqpub->retain) {
        array_add(&mqtb->topics, mqpub);
        // wtof("%s: topics=%u", __func__, array_count(&mqtb->topics));
    }

quit:
    if (lockrc==0) unlock(&mqtb->topics, LOCK_EXC);
    return rc;
}
