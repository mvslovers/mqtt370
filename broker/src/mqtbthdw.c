#include "mqtb370.h"

/* This is the MQTB wait thread implementation.
 * 
 * The main purpose is to wait for a session socket to be ready for 
 * read or write and then place that session into the thread manager
 * queue for processing by a worker thread.
 * 
 * When the session runnign on a worker thread encounters an EWOULDBLOCK
 * condition (read or write) then the worker thread should place the
 * session handle back into the wait queue and post the wait thread
 * so that it can rebuild the fd_set for all the sessions that are
 * waiting for socket activity.
 * 
 * Usage example:
 * CTHDTASK *task = 
 *      cthread_create_ex(mqtb_wait_thread, mqtb, NULL, 32*1024);
 */

static int build_fd_set(MQTB *mqtb, fd_set *read, fd_set *write, fd_set *excp);
static unsigned build_ecblist(MQTB *mqtb, unsigned **ecblist);
static int session_read(MQTB *mqtb, MQTBS *mqtbs);
static int session_send(MQTB *mqtb, MQTBS *mqtbs);

int
mqtb_wait_thread(void *arg1, void *arg2)
{
    MQTB        *mqtb   = (MQTB *)arg1;
	CLIBCRT     *crt    = __crtget();
    CLIBGRT     *grt    = __grtget();
    unsigned    *psa    = (unsigned *)0;
    unsigned    *tcb    = (unsigned *)psa[0x21C/4]; /* A(TCB) from PSATOLD  */
    unsigned    *ascb   = (unsigned *)psa[0x224/4]; /* A(ASCB) from PSAAOLD */
    CTHDTASK    *task   = cthread_self();
    CTHDMGR     *mgr;
    unsigned    *ecblist[10];
    int         rc;
    int         sock;
    int         len;
    MQTBS       *mqtbs;
    PB          *pb;
    int         maxsock;
    unsigned    count;
    unsigned    n;
    fd_set      readset;
    fd_set      writeset;
    struct sockaddr_in addr;
    struct sockaddr_in *a = (struct sockaddr_in *)&addr;

    wtof("MQTTD061I STARTING waiter thread    TCB(%06X) TASK(%06X) STACKSIZE(%u)",
        tcb, task, task->stacksize);

    /* if something goes all wrong, capture it! */
    abendrpt(ESTAE_CREATE, DUMP_DEFAULT);

    /* we loop until we're shut down */
    while(task) {
        memset(ecblist, 0, sizeof(ecblist));
        count = build_ecblist(mqtb, ecblist);
        // wtof("%s: ecblist has %u entries", __func__, count);

        /* build fd_set for select() */
        maxsock = build_fd_set(mqtb, &readset, &writeset, NULL);

        
        if (maxsock) {
            timeval wait;

            /* set the wait interval */
            wait.tv_sec     = 10;
            wait.tv_usec    = 0;

            // wtof("%s: calling selectex()", __func__);

            /* wait for socket ready or timeout */
            rc = selectex(maxsock, &readset, &writeset, NULL, &wait, ecblist);

            // wtof("%s: selectex() rc=%d", __func__, rc);
            
            /* check for selectex() error */
            if (rc<0) {
                wtof("MQTTD051E selectex() failed, rc=%d, error=%d", rc, errno);
                break;
            }
        }
        else {
            unsigned wait;  /* 1 = .01 seconds */
            
            /* set the wait time for 10 seconds */
            wait = 1000;

            // wtof("%s: calling cthread_timed_wait()", __func__);

            /* wait for ecb post or timer expire */
            rc = cthread_timed_wait(&mqtb->wait_ecb, wait, 0);
        }

        // wtof("%s: running", __func__);
        lock(&mqtb->wait_ecb, LOCK_EXC);
        mqtb->wait_ecb = 0;
        unlock(&mqtb->wait_ecb, LOCK_EXC);
        
        __64_add_u32(&mqtb->wait_disp, 1, &mqtb->wait_disp);
        
        /* check for quiesce or shutdown */
        if (mqtb->flags & MQTB_FLAG_QUIESCE) {
            if (maxsock==0) break; /* all done */
        }

        if (mqtb->flags & MQTB_FLAG_SHUTDOWN) break;

        mgr = mqtb->session_mgr;
        if (!mgr) continue; /* shouldn't happen, right */

        // wtof("%s: check wait queue", __func__);

        /* check which sessions need to recv or send */
        lock(&mqtb->wait_queue, LOCK_EXC);
        count = array_count(&mqtb->wait_queue);
        
        // if (!count) wtof("%s: wait queue empty", __func__);
        
        for(n=count; n > 0; n--) {
            mqtbs = array_get(&mqtb->wait_queue, n);

            // wtof("%s: checking session %p", __func__, mqtbs);
            
            if (!mqtbs) continue;
            // if (mqtbs->state != MQTBS_STATE_WAIT) continue;

            if (mqtb->flags & MQTB_FLAG_QUIESCE) goto closesession;

            rc = 0;
            if (FD_ISSET(mqtbs->sock, &readset)) {
                /* read packet for this session */
                rc = session_read(mqtb, mqtbs);
                if (rc < 0) goto checkerror;
            }

            /* if we're waiting for a response, don't process sends */
            if (!(mqtbs->flags & MQTBS_FLAG_WAITRESP)) {
                /* not waiting for a response, send is okay */
                if (mqtbs->spb || array_count(&mqtbs->spbq)) {
                    /* send packets for this session */
                    rc = session_send(mqtb, mqtbs);
                    if (rc < 0) goto checkerror;
                }
            }

            /* did we receive a complete packet buffer? */
            pb = mqtbs->rpb;
            if (pb && pb->pos == pb->bufsize) {
                // wtodumpf(pb, sizeof(PB), "%s: RECV PB %s", __func__, mqtb_packet_type(pb));
                /* yes, remove this session from the wait queue */
                array_del(&mqtb->wait_queue, n);

                /* update time of last packet read */
                time64(&mqtbs->read_time);

                /* move this session to session manager queue for processing */
                // wtof("-----------------------------------------------------");
                // wtof("%s: moving %p to session manager queue", __func__, mqtbs);
                cthread_queue_add(mgr, mqtbs);
                continue;
            }

checkerror:
            if (mqtbs->state != MQTBS_STATE_CLOSE) {
                mqtbs->state = MQTBS_STATE_WAIT;
            }

            if (mqtbs->flags & MQTBS_FLAG_ERROR) {
                mqtbs->state = MQTBS_STATE_CLOSE;
            }

            if (rc < 0) {
                if (errno != EWOULDBLOCK) {
                    mqtbs->state = MQTBS_STATE_CLOSE;
                }
            }

            if (mqtbs->state == MQTBS_STATE_CLOSE) {
closesession:
                /* remove this session from the wait queue */
                array_del(&mqtb->wait_queue, n);

                /* close this session */
                // wtof("%s: closing session %p", __func__, mqtbs);
                mqtb_session_close(mqtb, &mqtbs);
            }
        }
        unlock(&mqtb->wait_queue, LOCK_EXC);
        // wtof("%s: done with wait queue", __func__);
    } /* while(task) */

quit:
	wtof("MQTTD060I SHUTDOWN waiter thread    TCB(%06X) TASK(%06X) STACKSIZE(%u)",
		tcb, task, task->stacksize);

    abendrpt(ESTAE_DELETE, DUMP_DEFAULT);
    return 0;
}

static int
build_fd_set(MQTB *mqtb, fd_set *read, fd_set *write, fd_set *excp)
{
    int         maxsock = 0;
    unsigned    count   = 0; // array_count(&mqtb->wait_queue);
    unsigned    n;
    MQTBS       *mqtbs;

    if (read) {
        memset(read, 0, sizeof(fd_set));
    }
    if (write) {
        memset(write, 0, sizeof(fd_set));
    }
    if (excp) {
        memset(excp, 0, sizeof(fd_set));
    }

    lock(&mqtb->wait_queue, LOCK_SHR);
    count = array_count(&mqtb->wait_queue);
    for(n=0; n < count; n++) {
        mqtbs = mqtb->wait_queue[n];
        if (!mqtbs) continue;           /* no client handle? */

        if (mqtbs->sock < 0) continue;   /* no socket? */

        if (mqtbs->sock > maxsock) maxsock = mqtbs->sock;

        if (read)   FD_SET(mqtbs->sock, read);

        if (!(mqtbs->flags & MQTBS_FLAG_WAITRESP)) {
            if (write) {
                if (mqtbs->spb || array_count(&mqtbs->spbq)) {
                    FD_SET(mqtbs->sock, write);
                }
            }
        }

        if (excp)   FD_SET(mqtbs->sock, excp);
    }
    unlock(&mqtb->wait_queue, LOCK_SHR);

quit:
    if (maxsock) maxsock++;

    return maxsock;
}

static unsigned
build_ecblist(MQTB *mqtb, unsigned **ecblist)
{
    unsigned        pos     = 0;
    unsigned        *ecbp;

    ecblist[pos++] = &mqtb->wait_ecb;

quit:
    if (pos) {
        ecbp = ecblist[pos-1];
        ecblist[pos-1] = (unsigned*)((unsigned)ecbp | 0x80000000);
    }

    return pos;
}

static int session_read(MQTB *mqtb, MQTBS *mqtbs)
{
    int             rc      = 0;
    unsigned        bytes   = 0;
    unsigned        len     = 0;
    unsigned char   buf[6]  = {0};
    PB              *pb;

    mqtbs->state = MQTBS_STATE_READ;
    if (!mqtbs->rpb) {
        /* get the fixed header into buf */
        rc = mqtb_session_getc(mqtbs);
        if (rc < 0) goto quit;
        if (rc==0) {
            /* we're expecting the first byte of the fixed header
             * and we didn't get it. socket remote end probably
             * closed without sending any data.
             */
            rc = -1;
            goto quit;
        }
        buf[len++] = (unsigned char) rc;

        do {
            rc = mqtb_session_getc(mqtbs);
            if (rc < 0) goto quit;
            buf[len++] = (unsigned char) rc;
            if (rc < 0x80) break;
        } while(len < 6);

        // wtodumpf(buf, len, "%s: %p fixed header", __func__, mqtbs);

        /* decode the fixed header remaining length */
        rc = util_length_decode(buf, &len);
        
        // wtof("%s: fixed header len %d, remaining length %u", __func__, rc, len);
        
        bytes = (unsigned)rc + len + sizeof(PB);
        pb = mqtbs->rpb = calloc(1, bytes);
        if (!pb) {
            wtof("%s: No memory. calloc(1,%u)", __func__, bytes);
            rc = -1;
            goto quit;
        }
        
        pb->bufsize = (unsigned)rc + len;
        pb->pos     = (unsigned)rc;
        pb->varpos  = (unsigned short)rc;
        memcpy(pb->buf, buf, rc);
    }
    else {
        pb = mqtbs->rpb;
    }

    for(bytes = pb->bufsize - pb->pos; bytes; bytes = pb->bufsize - pb->pos) {
        rc = recv(mqtbs->sock, &pb->buf[pb->pos], bytes, 0);
        if (rc > 0) {
            pb->pos += (unsigned)rc;
        }
        else if (rc < 0) {
            goto quit;
        }
        else {
            /* nothing was read */
            rc = -1;
            goto quit;
        }
    
    }

quit:
    return rc;
}

static int session_send(MQTB *mqtb, MQTBS *mqtbs)
{
    int         rc;
    int         lockrc;
    PB          *pb;
    unsigned    count;
    const char  *type;
    unsigned char byte;
    unsigned char qos;
    time64_t    now;
    
    lockrc = lock(&mqtbs->spbq, LOCK_EXC);

    mqtbs->state = MQTBS_STATE_SEND;
    
    pb = mqtbs->spb;

    do {
        rc = 0;

        if (pb) {
            if (pb->pos == pb->bufsize) {
                /* this buffer already sent */
                // wtodumpf(pb, sizeof(PB), "%s: SEND PB %s", __func__, mqtb_packet_type(pb));
                
                /* does the packet we just sent require a response? */
                byte = pb->buf[0] & 0xF0;
                qos = (pb->buf[0] & 0x06) >> 1;

                switch (byte) {
                case TYPE_PUBLISH:
                    if (qos==0) break;
                    mqtbs->flags |= MQTBS_FLAG_WAITRESP;
                    // wtof("%s: waiting for %s response", __func__, (qos==1) ? "PUBACK" : "PUBREC");
                    break;
                case TYPE_PUBREC:
                    mqtbs->flags |= MQTBS_FLAG_WAITRESP;
                    // wtof("%s: waiting for %s response", __func__, "PUBREL");
                    break;
                case TYPE_PUBREL:
                    mqtbs->flags |= MQTBS_FLAG_WAITRESP;
                    // wtof("%s: waiting for %s response", __func__, "PUBCOMP");
                    break;
                }

                if (mqtbs->flags & MQTBS_FLAG_WAITRESP) {
                    /* set the retry time (now + 5 seconds) */
                    time64(&now);
                    __64_add_u32(&now, 5, &mqtbs->retry);

                    goto quit;
                }
               
                // wtof("%s: free(%p)", __func__, pb);
                free(pb);
                // wtof("%s: free(%p) OKAY", __func__, pb);
                pb = NULL;
                mqtbs->spb = NULL;
            }
        }
    
        if (!pb) {
            /* we don't have a packet buffer */
            count = array_count(&mqtbs->spbq);
            // wtof("%s: array_count(&mqtbs->spbq)=%u", __func__, count);
            // if (count) {
            //     wtodumpf(mqtbs->spbq, count*sizeof(unsigned), "%s: mqtbs->spbq", __func__);
            // }
            if (!count) goto quit;
    
            /* pop packet buffer from queue */
            pb = array_del(&mqtbs->spbq, 1);
            // wtof("%s: array_del(&mqtbs->spbq, 1)=%p", __func__, pb);
            // count = array_count(&mqtbs->spbq);
            // wtof("%s: array_count(&mqtbs->spbq)=%u", __func__, count);
            // if (count) {
            //     wtodumpf(mqtbs->spbq, count*sizeof(unsigned), "%s: mqtbs->spbq", __func__);
            // }
            
            if (!pb) goto quit;

            mqtbs->spb = pb;
            pb->pos = 0;
            
            type = mqtb_packet_type(pb);
            // wtodumpf(pb->buf, pb->bufsize, "SEND %s Packet Buffer", type);
        }

        /* we have a packet buffer */
        count = pb->bufsize - pb->pos;
        rc = send(mqtbs->sock, &pb->buf[pb->pos], count, 0);
        if (rc < 0) goto quit;
    
        pb->pos += rc;  /* update the position */
    } while(pb);
    
quit:
    if (lockrc==0) unlock(&mqtbs->spbq, LOCK_EXC);
    return rc;
}
