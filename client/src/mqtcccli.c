#include "mqtc370.h"

int mqtc_close_client(MQTC *mqtc) 
{
    int         rc      = 4;
    int         i;
    CTHDTASK    *task;
    
    if (!mqtc) goto quit;

    mqtc->flags |= MQTC_FLAG_SHUTDOWN;

    task = mqtc->task;
    if (task) {
        for(i=0; i < 10 && task->termecb == 0; i++) {
            /* task still running */
            cthread_post(&mqtc->task_ecb, 0);
            sleep(1);
            /* did thread terminate? */
            if (task->termecb & 0x40000000) break;
        }

        if (task->termecb==0) {
            /* task still running, kill the task */
            cthread_detach(task);
            cthread_yield();
        }

        /* delete the task thread */
        cthread_delete(&mqtc->task);
    }

    /* close the socket */
    if (mqtc->sock >= 0) {
        closesocket(mqtc->sock);
        mqtc->sock = -1;
    }

    /* close packet trace */
    if (mqtc->pkttrc) {
        fclose(mqtc->pkttrc);
        mqtc->pkttrc = NULL;
    }

    /* reset flags */
    mqtc->flags = 0;
    rc = 0;
    
quit:
    return rc;
}
