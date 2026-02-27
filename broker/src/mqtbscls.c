#include "mqtb370.h"

static void free_exp(MQEXP ***array);

int mqtb_session_close(MQTB *mqtb, MQTBS **pmqtbs)
{
    int         lockrc;
    MQTBS       *mqtbs;
    unsigned    n;
    unsigned    count;
    
    if (!pmqtbs) goto quit;
    mqtbs = *pmqtbs;
    if (!mqtbs) goto quit;

    wtof("MQTTD321I Closing client handle %p", mqtbs);
    // wtodumpf(mqtbs, sizeof(MQTBS), "%s: MQTBS", __func__);

    if (mqtbs->sock >= 0) {
        closesocket(mqtbs->sock);
        mqtbs->sock = -1;
    }
    
    lockrc = lock(&mqtbs->spbq, LOCK_EXC);
    if (mqtbs->spbq) {
        count = array_count(&mqtbs->spbq);
        for(n=count; n > 0; n--) {
            PB *pb = array_del(&mqtbs->spbq, n);
            
            if (!pb) continue;
            free(pb);
        }
        array_free(&mqtbs->spbq);
        mqtbs->spbq = NULL;
    }
    if (lockrc==0) unlock(&mqtbs->spbq, LOCK_EXC);
    
    util_free_char(&mqtbs->protocal_name);
    util_free_char(&mqtbs->will_msg);
    util_free_char(&mqtbs->will_topic);
    util_free_char(&mqtbs->password);
    util_free_char(&mqtbs->user);
    util_free_char(&mqtbs->clientid);
    util_free_char((char**)&mqtbs->spb);
    util_free_char((char**)&mqtbs->rpb);

    /* remove this session from the subscribers array */
    mqtb_unsubscribe_session(mqtbs->mqtb, mqtbs);

    /* remove this session from array of sessions */
    lockrc = lock(&mqtb->mqtbs, LOCK_EXC);
    count = array_count(&mqtb->mqtbs);
    for(n=count; n > 0; n--) {
        MQTBS *s = array_get(&mqtb->mqtbs, n);
        
        if (!s) continue;
        if (s == mqtbs) {
            array_del(&mqtb->mqtbs, n);
            break;
        }
    }
    if (lockrc==0) unlock(&mqtb->mqtbs, LOCK_EXC);

    free(mqtbs);
    *pmqtbs = NULL;

quit:
    return 0;
}
