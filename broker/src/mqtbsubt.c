#include "mqtb370.h"

int mqtb_subscribe_topic(MQTB *mqtb, MQTBS *mqtbs, char *topic, char *qos)
{
    int             rc      = 0;
    int             lockrc;
    MQSUB           *mqsub;
    unsigned        n, count;
    unsigned        sn, scount;

    /* we're going to force downgrade the qos for testing */
    // *qos = 0;
    
    lockrc = lock(&mqtb->subscribers, LOCK_EXC);
    count = array_count(&mqtb->subscribers);
    for(n=0; n < count; n++) {
        mqsub = mqtb->subscribers[n];
        
        if (!mqsub) continue;
        if (mqsub->qos != *qos) continue;
        if (!mqsub->topic) continue;
        if (strcmp(mqsub->topic, topic)!=0) continue;
        
        /* we found a subscription for this qos level and topic */

        /* add mqtbs (session handle) to this subscription */
        scount = array_count(&mqsub->mqtbs);
        for(sn=0; sn < scount; sn++) {
            MQTBS *s = mqsub->mqtbs[sn];
            if (!s) continue;
            /* if mqtbs is already subscribed to this topic */
            if (s==mqtbs) goto do_topics;
        }
        /* add mqtbs (session handle) to this topic subscription */
        array_add(&mqsub->mqtbs, mqtbs);
        goto do_topics;
    }
    
    /* new subsciption for this qos and topic */
    mqsub = calloc(1, sizeof(MQSUB));
    if (!mqsub) goto do_topics;
    
    /* initialize this subscription */
    strcpy(mqsub->eye, MQSUB_EYE);
    mqsub->qos      = *qos;
    mqsub->topic    = strdup(topic);
    array_add(&mqsub->mqtbs, mqtbs);
    
    /* add this subscription to subscribers array */
    array_add(&mqtb->subscribers, mqsub);

do_topics:
    if (lockrc==0) unlock(&mqtb->subscribers, LOCK_EXC);

    lockrc = lock(&mqtb->topics, LOCK_EXC);
    count = array_count(&mqtb->topics);
    for(n=0; n < count; n++) {
        MQPUB *mqpub = mqtb->topics[n];
        if (!mqpub) continue;
        
        if (mqtb_topic_match(mqpub->topic, topic)) {
            /* send this topic to this session */
            rc = mqtb_publish_subscriber(mqtb, mqpub, mqsub->qos, mqtbs);
        }
    }
    if (lockrc==0) unlock(&mqtb->topics, LOCK_EXC);

quit:
    return rc;
}
