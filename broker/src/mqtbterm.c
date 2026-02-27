#include "mqtb370.h"

static void close_fd_set(void);

int mqtb_terminate(MQTB *mqtb)
{
    MQTBC       *mqtbc  = mqtb->mqtbc;
    int         rc;
    int         i;
    CTHDTASK    *task;
    ECB         ecb;
    
    /* prepare to terminate */
    mqtb->flags |= MQTB_FLAG_QUIESCE;
    wtof("MQTTD002I Broker is QUIESCE");

    /* terminate the monitor thread */
    if (mqtb->monitor_thread) {
        cthread_post(&mqtb->monitor_ecb, 0);
        task = mqtb->monitor_thread;
        for(i=0; i < 10; i++) {
            if (task->termecb & 0x40000000) break;

            /* timeout waiting for task to terminate */
            wtof("MQTTD900I Waiting for %s to terminate", "monitor thread");

            /* wait 1 second (100 = 1 second) */
            cthread_timed_wait(&ecb, 100, 0);
        }

        if (!task->termecb) {
            /* thread didn't termimnate, detach thread to kill it */
            wtof("MQTTD901W Force terminaing %s", "monitor thread"); 
            cthread_detach(task);
            cthread_yield();
        }

        /* delete the listener thread */
        cthread_delete(&mqtb->monitor_thread);
    }


    /* terminate the listener thread */
    if (mqtb->listen_thread) {
        cthread_post(&mqtb->listen_ecb, 0);
        task = mqtb->listen_thread;
        for(i=0; i < 10; i++) {
            if (task->termecb & 0x40000000) break;

            /* timeout waiting for task to terminate */
            wtof("MQTTD900I Waiting for %s to terminate", "listener thread");

            /* wait 1 second (100 = 1 second) */
            cthread_timed_wait(&ecb, 100, 0);
        }

        if (!task->termecb) {
            /* thread didn't termimnate, detach thread to kill it */
            wtof("MQTTD901W Force terminaing %s", "listener thread"); 
            cthread_detach(task);
            cthread_yield();
        }

        /* delete the listener thread */
        cthread_delete(&mqtb->listen_thread);
    }

    if (mqtb->listen >= 0) {
        closesocket(mqtb->listen);
        mqtb->listen = -1;
    }

    if (mqtb->publish_thread) {
        cthread_post(&mqtb->publish_ecb, 0);
        task = mqtb->publish_thread;
        for(i=0; i < 10; i++) {
            if (task->termecb & 0x40000000) break;

            /* timeout waiting for task to terminate */
            wtof("MQTTD900I Waiting for %s to terminate", "publish thread");

            cthread_post(&mqtb->publish_ecb, 0);

            /* wait 1 second (100 = 1 second) */
            cthread_timed_wait(&ecb, 100, 0);
        }

        if (!task->termecb) {
            /* thread didn't termimnate, detach thread to kill it */
            wtof("MQTTD901W Force terminaing %s", "publish thread"); 
            cthread_detach(task);
            cthread_yield();
        }

        /* delete the listener thread */
        cthread_delete(&mqtb->publish_thread);
    }

    if (mqtb->wait_thread) {
        cthread_post(&mqtb->wait_ecb, 0);
        task = mqtb->wait_thread;
        for(i=0; i < 10; i++) {
            if (task->termecb & 0x40000000) break;

            /* timeout waiting for task to terminate */
            wtof("MQTTD900I Waiting for %s to terminate", "waiter thread");

            cthread_post(&mqtb->wait_ecb, 0);

            /* wait 1 second (100 = 1 second) */
            cthread_timed_wait(&ecb, 100, 0);
        }

        if (!task->termecb) {
            /* thread didn't termimnate, detach thread to kill it */
            wtof("MQTTD901W Force terminaing %s", "waiter thread"); 
            cthread_detach(task);
            cthread_yield();
        }

        /* delete the listener thread */
        cthread_delete(&mqtb->wait_thread);
    }

    /* terminate the session threads */
    if (mqtb->session_mgr) {
        cthread_manager_term(&mqtb->session_mgr);
    }

    /* save retained topics to dataset */
    if (mqtbc->retain_dataset) {
        rc = mqtb_retain_save(mqtb, mqtbc->retain_dataset);
    }

    mqtbc_free(&mqtb->mqtbc);

    if (mqtb->config) {
        lua_close(mqtb->config);
        mqtb->config = NULL;
    }

quit:
    /* close any sockets we created but didn't close before */
    close_fd_set();

    return 0;
}

/* close any sockets we created but didn't close before */
static void close_fd_set(void)
{
    CLIBGRT *grt    = __grtget();

    if (grt->grtsock) {
        unsigned count  = array_count(&grt->grtsock);
        if (count) wtof("MQTTD990I cleanup of %u sockets", count);
        while(count > 0) {
            count--;
            CLIBSOCK *s = grt->grtsock[count];
            if (!s) continue;
            if (s->socket >= 0) {
                wtof("MQTTD991I closing socket %d", s->socket);
                wtodumpf(s, sizeof(CLIBSOCK), "%d CLIBSOCK", s->socket);
                closesocket(s->socket);
            }
        }
    }
}
