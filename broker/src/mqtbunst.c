#include "mqtb370.h"

int mqtb_unsubscribe_topic(MQTB *mqtb, MQTBS *mqtbs, char *topic)
{
    int             rc      = 0;
    int             lockrc;
    MQSUB           *mqsub;
    unsigned        n, count;
    unsigned        sn, scount;

    // wtof("%s: mqtbs=%p topic=\"%s\"", __func__, mqtbs, topic);
    
    lockrc = lock(&mqtb->subscribers, LOCK_EXC);
    count = array_count(&mqtb->subscribers);
    // wtof("%s: count=%u", __func__, count);
    for(n=count; n > 0; n--) {
        mqsub = array_get(&mqtb->subscribers, n);
        
        // wtof("%s: mqsub=%p", __func__, mqsub);
        if (!mqsub) continue;
        if (!mqsub->topic) continue;
        // wtof("%s: mqsub->topic=\"%s\"", __func__, mqsub->topic);
        if (strcmp(mqsub->topic, topic)!=0) continue;
        
        /* we found a subscription for this topic */

        /* remove mqtbs (session handle) from this subscription */
        scount = array_count(&mqsub->mqtbs);
        for(sn=scount; sn > 0; sn--) {
            MQTBS *s = array_get(&mqsub->mqtbs, sn);

            if (!s) continue;

            // wtof("%s: s=%p", __func__, s);

            /* if mqtbs is subscribed to this topic */
            if (s==mqtbs) {
                // wtof("%s: deleting s=%p", __func__, s);
                array_del(&mqsub->mqtbs, sn);
            }
        }
        
        /* check for no more subscribers to this topic */
        scount = array_count(&mqsub->mqtbs);
        if (scount) continue;
        
        /* there are no subscribers to this topic */
        // wtof("%s: no more subscribers to topic=\"%s\"", __func__, mqsub->topic);
        array_del(&mqtb->subscribers, n);

        array_free(&mqsub->mqtbs);
        util_free_char(&mqsub->topic);
        free(mqsub);
    }



quit:
    if (lockrc==0) unlock(&mqtb->subscribers, LOCK_EXC);
    return rc;
}
