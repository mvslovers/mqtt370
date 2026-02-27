#include "mqtb370.h"

static int session_work(MQTB *mqtb, MQTBS *mqtbs);

int mqtb_session_thread(void *udata, CTHDWORK *work)
{
    CLIBCRT     *crt    = __crtget();           /* A(CLIBCRT) from TCBUSER  */
    CLIBGRT     *grt    = __grtget();
    MQTB        *mqtb   = grt->grtapp1;
    unsigned    *psa    = (unsigned *)0;
    unsigned    *tcb    = (unsigned *)psa[0x21C/4]; /* A(TCB) from PSATOLD  */
    unsigned    *ascb   = (unsigned *)psa[0x224/4]; /* A(ASCB) from PSAAOLD */
    CTHDTASK    *task   = cthread_self();
    CTHDMGR     *mgr    = work->mgr;
    int         rc      = 0;
    char        *data   = NULL;

    /* if something goes all wrong, capture it! */
    abendrpt(ESTAE_CREATE, DUMP_DEFAULT);

    wtof("MQTTD061I STARTING session(%06X)  TCB(%06X) TASK(%06X) STACKSIZE(%u)",
        work, tcb, task, task->stacksize);

    while(task) {
        if (work->state == CTHDWORK_STATE_SHUTDOWN) break;

        rc = cthread_worker_wait(work,&data);
        if (rc == CTHDWORK_POST_SHUTDOWN) break;

        if (rc == CTHDWORK_POST_REQUEST) {
            /* process request */
            if (data) {
                MQTBS *mqtbs = (MQTBS*)data;    /* session request  */
                MQPUB *mqpub = (MQPUB*)data;    /* publish request  */

                if (strcmp(mqtbs->eye, MQTBS_EYE)==0) {
                    /* session work request */
                    rc = session_work(mqtb, mqtbs);
                }
                else {
                    wtodumpf(data, 16, "%s unknown request", __func__);
                }
            }
        }
        else if (rc == CTHDWORK_POST_TIMER) {
            /* process timer */
        }
    }

quit:
	wtof("MQTTD060I SHUTDOWN session(%06X)  TCB(%06X) TASK(%06X) STACKSIZE(%u)",
		work, tcb, task, task->stacksize);

    abendrpt(ESTAE_DELETE, DUMP_DEFAULT);

    return 0;
}

static int session_work(MQTB *mqtb, MQTBS *mqtbs)
{
    int         rc          = 0;

    /* update dispatched count */
    /* process this session handle */
    __64_add_u32(&mqtbs->dispatched, 1, &mqtbs->dispatched);

    /* process this session handle */
    mqtbs->state = MQTBS_STATE_PROC;
    rc = mqtb_session_process(mqtb, mqtbs);

    mqtbs->state = MQTBS_STATE_WAIT;

    /* put this session back on the wait queue */
    lock(&mqtb->wait_queue, LOCK_EXC);
    array_add(&mqtb->wait_queue, mqtbs);
    unlock(&mqtb->wait_queue, LOCK_EXC);
    cthread_post(&mqtb->wait_ecb, 0);

quit:
    return rc;
}
