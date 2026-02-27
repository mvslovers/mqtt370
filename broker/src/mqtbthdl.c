#include "mqtb370.h"

/* This is the MQTB listenter thread implementation.
 * 
 * The main purpose is to wait for a connection from a MQTT client
 * and create a new session handle.
 * 
 * The new session handle is then added to the array of session handles
 * and also added to the wait queue. The wait thread is then posted to 
 * pickup the new session.  
 * 
 * The wait queue thread then waits for a socket read or write to be 
 * available for processing and it the adds the session to the thread 
 * manager work queue for processing.
 * 
 * When the worker thread session is blocked (no data ready for read,
 * or no space in socket send buffer) the session will be added back
 * to the wait queue for processing by the wait thread.
 * 
 * Usage example:
 * CTHDTASK *task = 
 *      cthread_create_ex(mqtb_listen_thread, mqtb, NULL, 32*1024);
 */

static int build_fd_set(MQTB *mqtb, fd_set *read, fd_set *write, fd_set *excp);

int
mqtb_listen_thread(void *arg1, void *arg2)
{
    MQTB        *mqtb   = (MQTB *)arg1;
	CLIBCRT     *crt    = __crtget();
    CLIBGRT     *grt    = __grtget();
    unsigned    *psa    = (unsigned *)0;
    unsigned    *tcb    = (unsigned *)psa[0x21C/4]; /* A(TCB) from PSATOLD  */
    unsigned    *ascb   = (unsigned *)psa[0x224/4]; /* A(ASCB) from PSAAOLD */
    CTHDTASK    *task   = cthread_self();
    CTHDMGR     *mgr;
    int         rc;
    int         sock;
    int         len;
    MQTBS       *mqtbs;
    timeval     wait;
    int         maxsock;
    unsigned    count;
    fd_set      read;
    struct sockaddr_in addr;
    struct sockaddr_in *a = (struct sockaddr_in *)&addr;
    char        ip[24];
    char        port[8];

    wtof("MQTTD061I STARTING listen thread    TCB(%06X) TASK(%06X) STACKSIZE(%u)",
        tcb, task, task->stacksize);

    /* if something goes all wrong, capture it! */
    abendrpt(ESTAE_CREATE, DUMP_DEFAULT);

    /* we loop until we're shut down */
    while(task) {
        /* build fd_set for select() */
        maxsock = build_fd_set(mqtb, &read, NULL, NULL);
        if (!maxsock) {
            wtof("MQTTD050E maxsock==0, something is wrong!");
            goto quit;
        }

        /* set the wait interval */
        wait.tv_sec     = 1;
        wait.tv_usec    = 0;

        /* wait for something to do */
        rc = selectex(maxsock, &read, NULL, NULL, &wait, NULL);

        /* check for selectex() error */
        if (rc<0) {
            wtof("MQTTD051E selectex() failed, rc=%d, error=%d", rc, errno);
            break;
        }

        /* check for quiesce or shutdown */
        if (mqtb->flags & MQTB_FLAG_QUIESCE) break;
        if (mqtb->flags & MQTB_FLAG_SHUTDOWN) break;

        /* check for new connection */
        if (FD_ISSET(mqtb->listen, &read)) {
            /* new client wants to connect */
            len = sizeof(addr);
            sock = accept(mqtb->listen, &addr, &len);
            if (sock<0) {
                wtof("MQTTD052E accept() failed, rc=%d, error=%d\n",
                    sock, errno);
                goto quit;
            }

            __64_add_u32(&mqtb->listen_disp, 1, &mqtb->listen_disp);

            len = 1;    /* set non-blocking I/O */
            if (ioctlsocket(sock, FIONBIO, &len)) {
                wtof("MQTTD053E Unable to set non-blocking I/O for socket %d\n",
                    sock);
            }

            /* allocate a new session handle */
            mqtbs = calloc(1, sizeof(MQTBS));
            if (!mqtbs) {
                wtof("MQTTD999E Out of memory!");
                goto quit;
            }

            rc = util_socket_peer(sock, ip, port);
            if (rc) {
                wtof("MQTTD320I Creating client handle %p", mqtbs);
            }
            else {
                wtof("MQTTD320I Creating client handle %p ip=%s port=%s", mqtbs, ip, port);
            }

            /* initialize the session handle */
            strcpy(mqtbs->eye, MQTBS_EYE);
            mqtbs->mqtb     = mqtb;
            mqtbs->sock     = sock;
            // mqtbs->addr     = a->sin_addr.s_addr;
            // mqtbs->port     = a->sin_port;
            mqtbs->state    = MQTBS_STATE_WAIT;

            /* add this new session to the sessions array */
            lock(&mqtb->mqtbs, LOCK_EXC);
            array_add(&mqtb->mqtbs, mqtbs);
            unlock(&mqtb->mqtbs, LOCK_EXC);
            
            /* put this new session to the wait queue */
            lock(&mqtb->wait_queue, LOCK_EXC);
            array_add(&mqtb->wait_queue, mqtbs);
            unlock(&mqtb->wait_queue, LOCK_EXC);
            
            /* wake up the wait queue thread */
            // wtof("%s: session %p on wait queue", __func__, mqtbs);
            lock(&mqtb->wait_ecb, LOCK_EXC);
            cthread_post(&mqtb->wait_ecb, 0);
            unlock(&mqtb->wait_ecb, LOCK_EXC);
        } /* if (FD_ISSET(mqtb->listen, &read)) */
    } /* while(task) */

quit:
	wtof("MQTTD060I SHUTDOWN listen thread    TCB(%06X) TASK(%06X) STACKSIZE(%u)",
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

    if (read && mqtb->listen >= 0) {
        FD_SET(mqtb->listen, read);
        if (mqtb->listen > maxsock) maxsock = mqtb->listen;
    }

quit:
    if (maxsock) maxsock++;

    return maxsock;
}
