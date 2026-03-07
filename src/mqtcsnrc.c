#include "mqtc370.h"

static unsigned build_ecblist(MQTC *mqtc, unsigned **ecblist);
static int session_getc(MQTC *mqtc);
static int session_read(MQTC *mqtc);
static int session_send(MQTC *mqtc);
static int check_set_wait_resp(MQTC *mqtc, CPB *pb);
static int reconnect_session(MQTC *mqtc);

int mqtc_send_recv(MQTC *mqtc)
{
    int         rc          = 0;
    CTHDTASK    *task       = mqtc->task;
    time64_t    now;
    unsigned    *ecblist[2];
    int         sock;
    int         len;
    CPB         *pb;
    int         maxsock;
    unsigned    count;
    unsigned    n;
    fd_set      readset;
    fd_set      writeset;
    struct sockaddr_in addr;
    struct sockaddr_in *a = (struct sockaddr_in *)&addr;

    do {
        ecblist[0] = ecblist[1] = 0;
        count = build_ecblist(mqtc, ecblist);

        /* build fd_set for select() */
        maxsock = 0;
        if (mqtc->sock >= 0) {
            maxsock = mqtc->sock + 1;
            memset(&readset, 0, sizeof(fd_set));
            memset(&writeset, 0, sizeof(fd_set));
            FD_SET(mqtc->sock, &readset);
            if (!(mqtc->flags & MQTC_FLAG_WAITRESP)) {
                if (mqtc->spb || array_count(&mqtc->spbq)) {
                    FD_SET(mqtc->sock, &writeset);
                }
            }
        }
    
        if (maxsock) {
            timeval wait;

            if (task) {
                /* set the wait interval */
                wait.tv_sec     = 10;
                wait.tv_usec    = 0;
            }
            else {
                /* set the wait interval */
                wait.tv_sec     = 0;
                wait.tv_usec    = 100;
            }
            // wtof("%s: calling selectex()", __func__);

            /* wait for socket ready or timeout */
            rc = selectex(maxsock, &readset, &writeset, NULL, &wait, ecblist);

            /* check for selectex() error */
            if (rc<0) {
                if (task) {
                    wtof("MQTC0051E selectex() failed, rc=%d, error=%d", rc, errno);
                }
                break;
            }
        }
        else {
            unsigned wait;  /* 1 = .01 seconds */

            if (task) {
                /* set the wait time for 10 seconds */
                wait = 1000;
            }
            else {
                /* set the wait time for .10 seconds */
                wait = 10;
            }

            // wtof("%s: calling cthread_timed_wait()", __func__);
    
            /* wait for ecb post or timer expire */
            rc = cthread_timed_wait(&mqtc->task_ecb, wait, 0);
        }

        // wtof("%s: running", __func__);
    
        /* clear the task ECB */
        lock(&mqtc->task_ecb, LOCK_EXC);
        mqtc->task_ecb = 0;
        unlock(&mqtc->task_ecb, LOCK_EXC);

        /* update the task dispatched counter */ 
        __64_add_u32(&mqtc->taskdisp, 1, &mqtc->taskdisp);
        
        /* get the current time */
        time64(&mqtc->tasktime);

        if (mqtc->flags & MQTC_FLAG_SHUTDOWN) break;

        if (mqtc->flags & MQTC_FLAG_ERROR) {
            /* attempt reconnect to broker */
            rc = reconnect_session(mqtc);
            break;
        }

        rc = 0;
        if (FD_ISSET(mqtc->sock, &readset)) {
            /* read packet for this session */
            rc = session_read(mqtc);
            if (rc < 0) goto checkerror;
        }

        /* if we're waiting for a response, don't process sends */
        if (!(mqtc->flags & MQTC_FLAG_WAITRESP)) {
            /* not waiting for a response, send is okay */
            if (mqtc->spb || array_count(&mqtc->spbq)) {
                /* send packets for this session */
                rc = session_send(mqtc);
                if (rc < 0) goto checkerror;
            }
        }

        /* did we receive a complete packet buffer? */
        pb = mqtc->rpb;
        if (pb && pb->pos == pb->bufsize) {
            // wtodumpf(pb->buf, pb->bufsize, "RECV %s", mqtc_packet_type(pb));
            if (mqtc->pkttrc) mqtc_packet_trace(mqtc, pb, 0);
            
            /* update time of last packet read */
            time64(&mqtc->readtime);

            /* process the received packet */
            rc = mqtc_session_process(mqtc);

            /* discard received packet */
            pb = mqtc->rpb;
            if (pb) {
                free(pb);
                mqtc->rpb = NULL;
            }

            if (rc < 0) goto checkerror;
        }

        /* check for WAITRESP */
        if (mqtc->flags & MQTC_FLAG_WAITRESP) {
            /* currently waiting for response */
            pb = mqtc->spb;
            if (pb && pb->pos == pb->bufsize) {
                /* sanity check - last packet was sent */
                time64(&now);
                if (__64_cmp(&mqtc->retry, &now)==__64_SMALLER) {
                    /* retry send of last packet */
                    pb->pos = 0;
                    __64_add_u32(&mqtc->retry, 5, &mqtc->retry);
                    /* remove the WAITRESP flag to resume sending */
                    mqtc->flags &= ~MQTC_FLAG_WAITRESP;
                }
            }
        }

        /* Check for retry of packets awaiting a response */
        lock(&mqtc->rpbq, LOCK_EXC);
        count = array_count(&mqtc->rpbq);
        if (count) {
            time64(&now);
            // wtodumpf(&now, sizeof(now), "%s: NOW", __func__);

            for(n=count; n > 0; n--) {
                RPB *rpb = array_get(&mqtc->rpbq, n);

                if (!rpb) continue;

                if (__64_cmp(&rpb->retry, &now)==__64_SMALLER) {
                    /* Time to resend this packet */
                    // wtodumpf(&rpb->retry, sizeof(time64_t), "%s: RETRY < NOW", __func__);
                    array_del(&mqtc->rpbq, n);
                    pb = rpb->cpb;
                    free(rpb);

                    /* check for previous resedn (DUP) */
                    if (pb->buf[0] & 0x08) {
                        /* DUP flag already set, give up */
                        mqtc->flags |= MQTC_FLAG_ERROR;
                        break;
                    }

                    /* Set the DUP flag and resend */
                    pb->buf[0] |= 0x08; /* set DUP flag */
                    mqtc_session_send(mqtc, pb);
                }
            }
        }
        unlock(&mqtc->rpbq, LOCK_EXC);

        break;

checkerror:
        if (rc < 0) {
            if (errno != EWOULDBLOCK) {
                /* close the socket */
                if (mqtc->sock >= 0) {
                    if (task) {
                        wtof("MQTC0058E rc=%d errno=%d closing socket=%d",
                        rc, errno, mqtc->sock);
                    }
                    closesocket(mqtc->sock);
                    mqtc->sock = -1;
                }
                /* set error fflag */
                mqtc->flags |= MQTC_FLAG_ERROR;
            }
        }
        else if (mqtc->flags & MQTC_FLAG_ERROR) {
            if (task) {
                wtof("MQTC0059E client error occured.");
            }
        }
    } while(0);

    rc = 0;
    
quit:
    return rc;
}

static unsigned
build_ecblist(MQTC *mqtc, unsigned **ecblist)
{
    unsigned        pos     = 0;
    unsigned        *ecbp;

    ecblist[pos++] = &mqtc->task_ecb;

quit:
    if (pos) {
        ecbp = ecblist[pos-1];
        ecblist[pos-1] = (unsigned*)((unsigned)ecbp | 0x80000000);
    }

    return pos;
}

static int 
session_getc(MQTC *mqtc)
{
    int             len;
    unsigned char   buf[4];

    len = recv(mqtc->sock, buf, 1, 0);
    if (len > 0) {
        /* we read a single character */
        len = (int) buf[0];
        goto quit;
    }

    if (len == 0) {
        /* nothing was read */
        // wtof("%s: recv() len=%d errno=%d", __func__, len, errno);
        len = -1; 
    }

    if (len < 0) {
        /* socket error, make sure errno is set */
        if (!errno) errno = EIO;
        // wtof("%s: recv() len=%d errno=%d", __func__, len, errno);
        goto quit;
    }

quit:
    return len;
}

static int 
session_read(MQTC *mqtc)
{
    int             rc      = 0;
    unsigned        bytes   = 0;
    unsigned        len     = 0;
    unsigned char   buf[6]  = {0};
    CPB             *pb;

    if (!mqtc->rpb) {
        /* get the fixed header into buf */
        rc = session_getc(mqtc);
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
            rc = session_getc(mqtc);
            if (rc < 0) goto quit;
            buf[len++] = (unsigned char) rc;
            if (rc < 0x80) break;
        } while(len < 6);

        // wtodumpf(buf, len, "%s: %p fixed header", __func__, mqtbs);

        /* decode the fixed header remaining length */
        rc = util_length_decode(buf, &len);
        
        // wtof("%s: fixed header len %d, remaining length %u", __func__, rc, len);
        
        bytes = (unsigned)rc + len + sizeof(CPB);
        pb = mqtc->rpb = calloc(1, bytes);
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
        pb = mqtc->rpb;
    }

    for(bytes = pb->bufsize - pb->pos; bytes; bytes = pb->bufsize - pb->pos) {
        rc = recv(mqtc->sock, &pb->buf[pb->pos], bytes, 0);
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


static int 
check_set_wait_resp(MQTC *mqtc, CPB *pb)
{
    int             rc          = 0;
    const char      *wait       = NULL;
    RPB             *rpb;
    time64_t        now;
    unsigned char   byte;
    unsigned char   qos;

    if (mqtc->flags & MQTC_FLAG_WAITRESP) goto quit;

    /* call the sent client packet callback */
    if (mqtc->f_scpb) {
       try(mqtc->f_scpb, mqtc, pb);
    }
                
    /* does the packet we sent require a response? */
    byte = pb->buf[0] & 0xF0;
    qos = (pb->buf[0] & 0x06) >> 1;

    if (byte == TYPE_CONNECT) {
        mqtc->flags |= MQTC_FLAG_WAITRESP;

        /* set the retry time (now + 5 seconds) */
        time64(&now);
        __64_add_u32(&now, 5, &mqtc->retry);

        goto quit;
    }

quit:
    return rc;
}

static int 
session_send(MQTC *mqtc)
{
    int         rc;
    int         lockrc;
    CPB         *pb;
    unsigned    count;
    const char  *type;
    
    lockrc = lock(&mqtc->spbq, LOCK_EXC);

    pb = mqtc->spb;

    do {
        rc = 0;

        if (pb) {
            if (pb->pos == pb->bufsize) {
                /* this buffer already sent */
                // wtodumpf(pb->buf, pb->bufsize, "SENT %s", mqtc_packet_type(pb));
                if (mqtc->pkttrc) mqtc_packet_trace(mqtc, pb, 1);

                /* check/set session wait response */
                check_set_wait_resp(mqtc, pb);

                /* if we're waiting for a response leave the packet as-is */
                if (mqtc->flags & MQTC_FLAG_WAITRESP) goto quit;
               
                /* we're not waiting for a response, discard packet */
                free(pb);
                mqtc->spb = pb = NULL;
            }
        }
    
        if (!pb) {
            /* we don't have a packet buffer */
            count = array_count(&mqtc->spbq);
            if (!count) goto quit;
    
            /* pop packet buffer from queue */
            pb = array_del(&mqtc->spbq, 1);
            
            if (!pb) goto quit;

            mqtc->spb = pb;
            pb->pos = 0;
        }

        /* we have a packet buffer */
        count = pb->bufsize - pb->pos;
        rc = send(mqtc->sock, &pb->buf[pb->pos], count, 0);
        if (rc < 0) goto quit;
    
        pb->pos += rc;  /* update the position */
    } while(pb);
    
quit:
    if (lockrc==0) unlock(&mqtc->spbq, LOCK_EXC);
    return rc;
}

static int 
reconnect_session(MQTC *mqtc)
{
    int         rc      = 4;
    CPB         *pb;
    int         sock;
    unsigned    n, count;
    int         error;
    const char  *errmsg;
    
    if (mqtc->flags & MQTC_FLAG_SHUTDOWN) goto quit;
    
    /* reset flags */
    mqtc->flags &= ~(MQTC_FLAG_ERROR | MQTC_FLAG_SESSION | MQTC_FLAG_WAITRESP);

    /* we're doing reconnect processing */
    mqtc->flags |= MQTC_FLAG_RECONNECT;

    /* close socket */
    if (mqtc->sock >= 0) {
        closesocket(mqtc->sock);
        mqtc->sock = -1;
    }

    /* reset client (packet buffers) */
    mqtc_reset_client(mqtc);

    /* open a non-blocking socket connected to MQTT broker */
    errno = 0;
    sock = util_open_nb_socket(mqtc->broker_host, mqtc->broker_port);
    if (sock < 0) {
        error = errno;

        if (mqtc->task) {
            wtof("MQTC1000W Unable to open socket to MQTT broker");
            if (error) {
                errmsg = strerror(error);
                if (!errmsg || !*errmsg) errmsg = "unknown error";
                wtof("MQTC1001W error=%d \"%s\"", error, errmsg);
            }
        }
        /* set the ERROR flag so we get another chance at it later */
        mqtc->flags |= MQTC_FLAG_ERROR;
        goto quit;
    }

    /* success, send CONNECT to broker */
    mqtc->sock = sock;
    
    if (mqtc->task) {
        wtof("MQTC1002I Reconnecting to broker=\"%s\" port=\"%s\"", 
            mqtc->broker_host, mqtc->broker_port);
    }
    rc = mqtc_connect(mqtc);

#if 0   /* put this logic in CONNACT processing */
    /* callback for reconnect */
    if (mqtc->f_reconnect) try(mqtc->f_reconnect, mqtc);

    /* reset the RECONNECT flag */
    mqtc->flags &= ~MQTC_FLAG_RECONNECT;
#endif

quit:
    return rc;
}
