#include "mqtb370.h"

static int publish_broker(MQTB *mqtb, time64_t *now);
static int publish_config(MQTB *mqtb, MQTBC *mqtbc);
static int publish_threads(MQTB *mqtb, time64_t *now);
static int publish_task(MQTB *mqtb, CTHDTASK *task, __64 *disp, const char *name, time64_t *now);
static int publish_main(MQTB *mqtb, time64_t *now);
static int publish_worker(MQTB *mqtb, time64_t *now);
static int retry_packets(MQTB *mqtb, time64_t *now);
static int expire_topics(MQTB *mqtb, time64_t *now);

extern unsigned __stklen;

int
mqtb_monitor_thread(void *arg1, void *arg2)
{
    MQTB        *mqtb   = (MQTB *)arg1;
	CLIBCRT     *crt    = __crtget();
    CLIBGRT     *grt    = __grtget();
    unsigned    *psa    = (unsigned *)0;
    unsigned    *tcb    = (unsigned *)psa[0x21C/4]; /* A(TCB) from PSATOLD  */
    unsigned    *ascb   = (unsigned *)psa[0x224/4]; /* A(ASCB) from PSAAOLD */
    CTHDTASK    *task   = cthread_self();
    time64_t    next_broker;    /* publish broker topics */
    time64_t    next_retry;     /* retry packets that require a response */
    time64_t    next_expire;    /* expire topics */
    unsigned    wait;           /* 1 = .01 seconds */
    int         rc;
    unsigned    count;
    unsigned    n;
    time64_t    now;

    wtof("MQTTD061I STARTING monitor thread   TCB(%06X) TASK(%06X) STACKSIZE(%u)",
        tcb, task, task->stacksize);

    /* if something goes all wrong, capture it! */
    abendrpt(ESTAE_CREATE, DUMP_DEFAULT);

    time64(&now);
    __64_add_u32(&now, 1, &next_broker);
    __64_add_u32(&now, 1, &next_retry);
    __64_add_u32(&now, 5, &next_expire);
    
    /* we loop until we're shut down */
    while(1) {
        /* update dispatched counter */
        __64_add_u32(&mqtb->monitor_disp, 1, &mqtb->monitor_disp);

        /* get current time */
        time64(&now);
        
        if (__64_cmp(&next_broker, &now)==__64_SMALLER) {
            /* publish broker taopics */
            // wtof("%s: --> publish_broker()", __func__);
            publish_broker(mqtb, &now);
            __64_add_u32(&now, 10, &next_broker);
        }

        if (__64_cmp(&next_retry, &now)==__64_SMALLER) {
            /* retry packets that require a response */
            // wtof("%s: --> retry_packets()", __func__);
            retry_packets(mqtb, &now);
            __64_add_u32(&now, 2, &next_retry);
        }

        if (__64_cmp(&next_expire, &now)==__64_SMALLER) {
            /* expire topics */
            // wtof("%s: --> expire_topics()", __func__);
            expire_topics(mqtb, &now);
            __64_add_u32(&now, 5, &next_expire);
        }

        /* set the wait time for 1 seconds */
        wait = 100;

        // wtof("%s: calling cthread_timed_wait()", __func__);

        /* wait for ecb post or timer expire */
        // wtof("%s: --> cthread_timed_wait()", __func__);
        cthread_timed_wait(&mqtb->monitor_ecb, wait, 0);
        mqtb->monitor_ecb = 0;

        // wtof("%s: running", __func__);
        /* check for quiesce or shutdown */
        if (mqtb->flags & MQTB_FLAG_QUIESCE) break;
        if (mqtb->flags & MQTB_FLAG_SHUTDOWN) break;
    } /* while(1) */

quit:
	wtof("MQTTD060I SHUTDOWN monitor thread   TCB(%06X) TASK(%06X) STACKSIZE(%u)",
		tcb, task, task->stacksize);

    abendrpt(ESTAE_DELETE, DUMP_DEFAULT);
    return 0;
}

static int 
publish_broker(MQTB *mqtb, time64_t *now)
{
    struct tm   *tm;
    char        timebuf1[24];
    char        timebuf2[24];

    tm = gmtime64(now);
    strftime(timebuf1, sizeof(timebuf1), "%Y-%m-%d %H:%M:%S", tm);
    tm = localtime64(now);
    strftime(timebuf2, sizeof(timebuf2), "%Y-%m-%d %H:%M:%S", tm);

    mqtb_pubf(mqtb, "$SYS/broker/name", "MQTTD - MQTT370");
    mqtb_pubf(mqtb, "$SYS/broker/version", "1.0");
    mqtb_pubf(mqtb, "$SYS/broker/protocol", "%s", mqtb->proto);

    if (mqtb->mqtbc) publish_config(mqtb, mqtb->mqtbc);

    mqtb_pubf(mqtb, "$SYS/broker/ready",
        "{ \"gmt\" : \"%s\" "
        ", \"local\" : \"%s\" "
        "}",
        timebuf1, 
        timebuf2);

    publish_threads(mqtb, now);
}

static int 
publish_config(MQTB *mqtb, MQTBC *mqtbc)
{
    mqtb_pubf(mqtb, "$SYS/broker/config", 
        "{ \"bind_tries\" : %u "
        ", \"listenq\" : %u "
        ", \"maxtask\" : %u "
        ", \"mintask\" : %u "
        ", \"port\" : %u "
        ", \"topic_expire\" : %u "
        ", \"tz\" : \"%s\" "
        "}",
        mqtbc->bind_tries,
        mqtbc->listenq,
        mqtbc->maxtask,
        mqtbc->mintask,
        mqtbc->port,
        mqtbc->topic_expire,
        mqtbc->tz);

    return 0;
}

static int 
publish_threads(MQTB *mqtb, time64_t *now)
{
    publish_main(mqtb, now);
    publish_task(mqtb, mqtb->wait_thread, &mqtb->wait_disp, "waiter", now);
    publish_task(mqtb, mqtb->listen_thread, &mqtb->listen_disp, "listener", now);
    publish_task(mqtb, mqtb->monitor_thread, &mqtb->monitor_disp, "monitor", now);
    publish_task(mqtb, mqtb->publish_thread, &mqtb->publish_disp, "publisher", now);
    publish_worker(mqtb, now);

    return 0;
}

static int 
publish_main(MQTB *mqtb, time64_t *now)
{
    CLIBPPA     *ppa    = __ppaget();   /* A(CLIBPPA)               */
    CTHDTASK    *task   = mqtb->main;
    unsigned    stk     = 0;
    unsigned    bytes   = 0;
    unsigned    hwm;
    char        timebuf[28];
    
    stk = (unsigned)ppa + sizeof(CLIBPPA);
    bytes = __stklen;
    hwm = util_stack_hwm((void*)stk, bytes);

    mqtb_fmt_datetime(mqtb, now, timebuf);
    
    mqtb_pubf(mqtb, "$SYS/broker/main", 
        "{ \"datetime\" : \"%s\" "
        ", \"task\" : \"%06X\" "
        ", \"tcb\" : \"%06X\" "
        ", \"stack\" : \"%06X\" "
        ", \"size\" : %u "
        ", \"hwm\" : %u "
        "}",
        timebuf,
        task, 
        task->tcb,
        stk,
        bytes,
        hwm);
    
    return 0;
}

static int 
publish_worker(MQTB *mqtb, time64_t *now)
{
    CTHDMGR     *mgr    = mqtb->session_mgr;
    CTHDTASK    *task   = mgr->task;
    unsigned    workcnt = 0;
    CTHDWORK    *work;
    unsigned    stk     = 0;
    unsigned    bytes   = 0;
    unsigned    hwm;
    char        topic[40];
    char        timebuf[28];
    unsigned    n, count;
    
    hwm = util_task_hwm(task, &stk, &bytes);

    mqtb_fmt_datetime(mqtb, now, timebuf);
    
    mqtb_pubf(mqtb, "$SYS/broker/worker", 
        "{ \"datetime\" : \"%s\" "
        ", \"dispatched\" : %llu "
        ", \"task\" : \"%06X\" "
        ", \"tcb\" : \"%06X\" "
        ", \"stack\" : \"%06X\" "
        ", \"size\" : %u "
        ", \"hwm\" : %u "
        "}",
        timebuf,
        mgr->dispatched,
        task, 
        task->tcb,
        stk,
        bytes,
        hwm);

    count = array_count(&mgr->worker);
    for(n=0; n < count; n++) {
        work = mgr->worker[n];
        
        if (!work) continue;
        task = work->task;
        if (!task) continue;
        
        hwm = util_task_hwm(task, &stk, &bytes);
        mqtb_fmt_datetime(mqtb, &work->disp_time, timebuf);

        sprintf(topic, "$SYS/broker/worker/%u", ++workcnt);
        mqtb_pubf(mqtb, topic, 
            "{ \"datetime\" : \"%s\" "
            ", \"dispatched\" : %llu "
            ", \"task\" : \"%06X\" "
            ", \"tcb\" : \"%06X\" "
            ", \"stack\" : \"%06X\" "
            ", \"size\" : %u "
            ", \"hwm\" : %u "
            "}",
            timebuf,
            work->dispatched,
            task, 
            task->tcb,
            stk,
            bytes,
            hwm);
    }

    return 0;
}


static int 
publish_task(MQTB *mqtb, CTHDTASK *task, __64 *disp, const char *name, time64_t *now)
{
    unsigned    stk     = 0;
    unsigned    bytes   = 0;
    unsigned    hwm;
    char        topic[40];
    char        timebuf[28];
    
    sprintf(topic, "$SYS/broker/thread/%s", name);

    hwm = util_task_hwm(task, &stk, &bytes);

    mqtb_fmt_datetime(mqtb, now, timebuf);
    
    mqtb_pubf(mqtb, topic, 
        "{ \"datetime\" : \"%s\" "
        ", \"dispatched\" : %llu "
        ", \"task\" : \"%06X\" "
        ", \"tcb\" : \"%06X\" "
        ", \"stack\" : \"%06X\" "
        ", \"size\" : %u "
        ", \"hwm\" : %u "
        "}",
        timebuf,
        *disp,
        task, 
        task->tcb,
        stk,
        bytes,
        hwm);
    
    return 0;
}

static int 
retry_packets(MQTB *mqtb, time64_t *now)
{
    unsigned        post_waiter = 0;
    unsigned        n, count;
    MQTBS           *mqtbs;
    PB              *pb;
    
    lock(&mqtb->wait_queue, LOCK_EXC);
    count = array_count(&mqtb->wait_queue);
    for(n=0; n < count; n++) {
        mqtbs = mqtb->wait_queue[n];
        if (!mqtbs) continue;           /* no client handle? */

        /* is this packet is waiting for a response? */
        if (mqtbs->flags & MQTBS_FLAG_WAITRESP) {
            /* yes, is it time for a retry? */
            if (__64_cmp(&mqtbs->retry, now)==__64_LARGER) {
                /* no, look at next session */
                continue;
            }
            
            /* yes, get current packet buffer */
            pb = mqtbs->spb;

            /* sanity check */
            if (!pb) continue;
            if (pb->pos != pb->bufsize) continue;

            /* has this packet already been resent before? */
            if (pb->buf[0] & 0x08) {    /* 0x08 == DUP flag */
                /* yes, we didn't get a response so close the session */
                wtof("%s: ERROR: resend of duplicate packet", __func__);
                mqtbs->flags |= MQTBS_FLAG_ERROR;
                mqtbs->state = MQTBS_STATE_CLOSE;
                post_waiter = 1;
                continue;
            }

            wtof("%s: resending packet with DUP flag", __func__);
            /* we need to resend this packet */
            /* set the DUP flag in the packet */
            pb->buf[0] |= 0x08; 
            /* reset the packet position for resend */
            pb->pos = 0;        
            /* reset the WAITDISP flag to resend the packet */
            mqtbs->flags &= ~MQTBS_FLAG_WAITRESP;
            
            post_waiter = 1;
        }
    }
    unlock(&mqtb->wait_queue, LOCK_EXC);

    if (post_waiter) {
        /* wake up the waiter thread */
        lock(&mqtb->wait_ecb, LOCK_EXC);
        cthread_post(&mqtb->wait_ecb, 0);
        unlock(&mqtb->wait_ecb, LOCK_EXC);
    }

    return 0;
}

static int 
expire_topics(MQTB *mqtb, time64_t *now)
{
    unsigned        n, count;
    MQPUB           *pub;
    
    lock(&mqtb->topics, LOCK_EXC);
    count = array_count(&mqtb->topics);
    for(n=count; n > 0; n--) {
        pub = array_get(&mqtb->topics, n);
        if (!pub) continue;

        if (__64_cmp(&pub->expires, now)==__64_SMALLER) {
            /* expired topic */
            wtof("MQTTD408I Discarding expired topic=\"%s\"", pub->topic);
            array_del(&mqtb->topics, n);
            mqtb_publish_free(mqtb, &pub);
        }
    }
    unlock(&mqtb->topics, LOCK_EXC);

    return 0;
}

