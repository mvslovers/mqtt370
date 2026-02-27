#include "mqtb370.h"

static int already_done(MQTBS ***pdone, MQTBS *mqtbs);

int mqtb_publish_request(MQTB *mqtb, MQPUB *mqpub)
{
    int         rc      = 0;
    MQTBS       **done  = NULL;
    int         lockrc;
    unsigned    n;
    unsigned    count;
    unsigned    sn;
    unsigned    scount;

    // wtof("%s: %s", __func__, mqpub->topic);
    // wtodumpf(mqpub, sizeof(MQPUB), "%s: MQPUB", __func__);
    // if (mqpub->msg && mqpub->msg_len) {
    //     wtodumpf(mqpub->msg, mqpub->msg_len, "%s: MSG", __func__);
    // }

    /* add/update/delete the published topic */
    rc = mqtb_publish_update(mqtb, mqpub);

    lockrc = lock(&mqtb->subscribers, LOCK_EXC);

    count = array_count(&mqtb->subscribers);
    for(n=0; n < count; n++) {
        MQSUB *mqsub = mqtb->subscribers[n];
        
        if (!mqsub) continue;

        if (mqtb_topic_match(mqpub->topic, mqsub->topic)) {
            /* publish topic to subscribers */
            scount = array_count(&mqsub->mqtbs);
            for(sn=0; sn < scount; sn++) {
                MQTBS *mqtbs = mqsub->mqtbs[sn];
                
                if (!mqtbs) continue;
                
                /* have we've published topic to this session? */
                if (already_done(&done, mqtbs)) continue;   /* yes */

                /* publish topic to subscriber session */
                rc = mqtb_publish_subscriber(mqtb, mqpub, mqsub->qos, mqtbs);
                array_add(&done, mqtbs);
            }
            break;
        }
    }

    if (!mqpub->retain) {
        /* we didn't add this topic so it's safe to free */
        mqtb_publish_free(mqtb, &mqpub);
    }

quit:
    if (lockrc==0) unlock(&mqtb->subscribers, LOCK_EXC);
    if (done) array_free(&done);
    
    return rc;
}

static int already_done(MQTBS ***pdone, MQTBS *mqtbs)
{
    MQTBS       **done      = *pdone;
    unsigned    n, count;
    
    count = array_count(&done);
    for(n=0; n < count; n++) {
        MQTBS *p = done[n];
        
        if (p == mqtbs) return 1;
    }
    
    return 0;
}
