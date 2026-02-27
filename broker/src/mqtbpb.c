#include "mqtb370.h"

int mqtb_pub(MQTB *mqtb, const char *topic, void *msg, unsigned len)
{
    int         rc      = 12;
    MQPUB       *mqpub;
    
    if (!mqtb) goto quit;
    if (!topic) goto quit;

    if (mqtb->flags & MQTB_FLAG_QUIESCE || 
        mqtb->flags & MQTB_FLAG_SHUTDOWN) {
        goto quit;
    }

    mqpub = calloc(1, sizeof(MQPUB));
    if (!mqpub) {
        wtof("%s: Unable to allocate %u bytes", __func__, sizeof(MQPUB));
        goto quit;
    }
    
    strcpy(mqpub->eye, MQPUB_EYE);
    mqpub->retain   = 1;
    mqpub->qos      = 1;
    mqpub->topic    = strdup(topic);
    if (!mqpub->topic) {
        wtof("%s: Unable to allocate %u bytes", __func__, strlen(topic)+1);
        goto cleanup;
    }

    if (mqpub->qos) {
        /* need to assign a message id */
        mqpub->msgid = (unsigned short) util_prng();
    }

    if (msg && len) {
        mqpub->msg  = util_memdup(msg, len);
        if (!mqpub->msg) {
            wtof("%s: Unable to allocate %u bytes", __func__, len);
            goto cleanup;
        }
        mqpub->msg_len = len;
    }

    lock(&mqtb->publish_queue, LOCK_EXC);
    array_add(&mqtb->publish_queue, mqpub);
    unlock(&mqtb->publish_queue, LOCK_EXC);
    
    lock(&mqtb->publish_ecb, LOCK_EXC);
    cthread_post(&mqtb->publish_ecb, 1);
    unlock(&mqtb->publish_ecb, LOCK_EXC);
    
    goto quit;

cleanup:
    if (mqpub) {
        util_free_char(&mqpub->topic);
        util_free_char((char **)&mqpub->msg);
        free(mqpub);
    }

quit:
    // wtof("%s: exit rc=%d", __func__, rc);
    return rc;
}
