#include "mqtb370.h"

static int publish_work(MQTB *mqtb, MQPUB *mqpub);

int mqtb_publish_thread(void *arg1, void *arg2)
{
    MQTB        *mqtb   = (MQTB *)arg1;
    CLIBCRT     *crt    = __crtget();           /* A(CLIBCRT) from TCBUSER  */
    CLIBGRT     *grt    = __grtget();
    unsigned    *psa    = (unsigned *)0;
    unsigned    *tcb    = (unsigned *)psa[0x21C/4]; /* A(TCB) from PSATOLD  */
    unsigned    *ascb   = (unsigned *)psa[0x224/4]; /* A(ASCB) from PSAAOLD */
    CTHDTASK    *task   = cthread_self();
    int         rc      = 0;
    MQPUB       *mqpub;
    unsigned    wait;
    unsigned    count;

    /* if something goes all wrong, capture it! */
    abendrpt(ESTAE_CREATE, DUMP_DEFAULT);

    wtof("MQTTD061I STARTING publish thread   TCB(%06X) TASK(%06X) STACKSIZE(%u)",
        tcb, task, task->stacksize);

    while(task) {
        if (mqtb->flags & MQTB_FLAG_SHUTDOWN) break;

        mqpub = NULL;
        
        lock(&mqtb->publish_queue, LOCK_EXC);
        count = array_count(&mqtb->publish_queue);
        if (count) mqpub = array_del(&mqtb->publish_queue, 1);
        unlock(&mqtb->publish_queue, LOCK_EXC);
        
        if (mqpub) {
            __64_add_u32(&mqtb->publish_disp, 1, &mqtb->publish_disp);
            
            if (strcmp(mqpub->eye, MQPUB_EYE)==0) {
                /* publish work request */
                rc = publish_work(mqtb, mqpub);
            }
            else {
                wtodumpf(mqpub, 16, "%s unknown request", __func__);
            }
            continue;
        }

        /* publish queue is empty */
        
        if (mqtb->flags & MQTB_FLAG_QUIESCE) break;

        /* set the wait time for 10 seconds */
        wait = 1000;

        /* wait for ecb post or timer expire */
        cthread_timed_wait(&mqtb->publish_ecb, wait, 0);
    }

quit:
	wtof("MQTTD060I SHUTDOWN publish thread   TCB(%06X) TASK(%06X) STACKSIZE(%u)",
		tcb, task, task->stacksize);

    abendrpt(ESTAE_DELETE, DUMP_DEFAULT);

    return 0;
}

static int publish_work(MQTB *mqtb, MQPUB *mqpub)
{
    int         rc      = 0;

    /* check for shutdown */
    if (mqtb->flags & MQTB_FLAG_QUIESCE) {
        mqtb_publish_free(mqtb, &mqpub);
        goto quit;
    }

    /* process this publish request */
    rc = mqtb_publish_request(mqtb, mqpub);

quit:
    return rc;
}
