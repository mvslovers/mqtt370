#include "mqttcli.h"

static int connect_cb(MQTC *mqtc, CPB *pb);
static int process_connack(MQTC *mqtc, CPB *pb);

int mqtt_connect(MQTTCLI *mqttcli)
{
    int         rc      = 8;
    MQTC        *mqtc   = mqttcli->mqtc;
    void        *rcpb   = mqtc->f_rcpb;
    int         sock    = mqtc ? mqtc->sock : -1;
    unsigned    wait;   /* 1 = .01 seconds */
    int         tries;

    // printf("%s enter sock=%d\n", __func__, sock);

    if (sock < 0) {
        /* no socket */
        if (!mqttcli->host) {
            printf("Connection failed. No MQTT host exist\n");
            goto quit;
        }
        if (!mqttcli->port) {
            printf("Connection failed. No MQTT port exist\n");
            goto quit;
        }
        
        rc = mqtc_open_client(mqtc, mqttcli->host, mqttcli->port);
        if (rc) {
            printf("Connection failed for MQTT host=%s:%s\n", 
                mqttcli->host, mqttcli->port);
            goto quit;
        }
        
        sock = mqtc->sock;
        printf("Socket opened for host=%s:%s socket=%d\n", 
            mqttcli->host, mqttcli->port, sock);
        rc = 0;
    }

    if (!(mqttcli->flags & MQTTCLI_FLAG_CONN)) {
        /* no connection established */
        // printf("%s: !MQTTCLI_FLAG_CONN\n", __func__);

        if (!mqtc->task) {
            mqttcli->flags |= MQTTCLI_FLAG_POLL;
            // printf("%s: mqttcli->flags |= MQTTCLI_FLAG_POLL\n", __func__);
        }
        
        if (mqttcli->user && !mqtc->userid) {
            rc = mqtc_set_userid(mqtc, mqttcli->user);
            if (rc) {
                printf("Connection failed to set userid\n");
                goto quit;
            }
        }
        
        if (mqttcli->pass && !mqtc->password) {
            rc = mqtc_set_password(mqtc, mqttcli->pass);
            if (rc) {
                printf("Connection failed to set password\n");
                goto quit;
            }
        }

        /* add callback for CONNECT response */
        mqtc->f_rcpb = connect_cb;
        mqtc->ctx    = mqttcli;
        mqttcli->ecb = 0;

        /* send CONNECT request */
        rc = mqtc_connect(mqtc);
        if (rc) {
            printf("Send CONNECT failed rc=%d\n", rc);
            goto quit;
        }
        
        for(tries=0, rc=1; tries < 10 && rc == 1; tries++) {
            // printf("tries=%d\n", tries);
            if (mqttcli->flags & MQTTCLI_FLAG_POLL) {
                rc = mqtc_send_recv(mqtc);
                // printf("mqtc_send_recv(mqtc) rc=%d\n", rc);
                if (rc) {
                    printf("Send/Recv failed\n");
                    goto quit;
                }
            }
        
            /* set the wait time for 0.5 seconds */
            wait = 50;

            /* wait for ecb post or timer expire */
            rc = cthread_timed_wait(&mqttcli->ecb, wait, 1);
            // printf("cthread_timed_wait(&mqttcli->ecb, wait, 1) rc=%d\n", rc);
            if (rc==1) continue;    /* timeout */

            if (mqttcli->flags & MQTTCLI_FLAG_CONN) {
                /* connection success */
                rc = 0;
            }
            else {
                /* connection failed */
                rc = 8;
            }
            break;
        }
        
        if (rc==1) {
            /* timed out */
            printf("Connection attempt timed out.\n");
        }
    }

quit:
    mqtc->f_rcpb = rcpb;
    // printf("%s exit rc=%d\n", __func__, rc);
    return rc;
}

static int connect_cb(MQTC *mqtc, CPB *pb)
{
    int         rc          = 0;
    MQTTCLI     *mqttcli    = mqtc->ctx;
    unsigned    type;
    
    // wtodumpf(pb, sizeof(CPB), "%s: CPB", __func__);

    type = (pb->buf[0] & 0xF0);
    if (type == TYPE_CONNACK) { // 0x20
        rc = process_connack(mqtc, pb);
        if (rc==0) {
            /* Success */
            mqttcli->flags |= MQTTCLI_FLAG_CONN;
        }
        /* post waiting thread */
        cthread_post(&mqttcli->ecb, 0);
    }

quit:
    return rc;
}

static int process_connack(MQTC *mqtc, CPB *pb)
{
    MQTTCLI         *mqttcli    = mqtc->ctx;
    int             rc          = 0;
    unsigned        len         = 0;
    unsigned        pos;
    unsigned char   connflags;
    unsigned char   connrc;
    const char      *errmsg;
    
    // mqtc_packet_dump(mqtc, pb, "CONNACK");

    /* decode variable header */
    if (!pb->varpos) {
        rc = util_length_decode(pb->buf, &len);
        if (!rc) goto quit;
        /* start of variable header */
        pb->varpos  = (unsigned short)rc;
    }
    pos = pb->varpos;

    rc = 8;

    connflags = pb->buf[pos++];
    connrc    = pb->buf[pos++];
    
    if (connrc!=0) {
        /* connection failed */
        switch(connrc) {
        default:    errmsg = NULL; break;
        case 0x01:  errmsg = "Connection Refused: unacceptable protocol version"; break;
        case 0x02:  errmsg = "Connection Refused: identifier rejected"; break;
        case 0x03:  errmsg = "Connection Refused: server unavailable"; break;
        case 0x04:  errmsg = "Connection Refused: bad user name or password"; break;
        case 0x05:  errmsg = "Connection Refused: not authorized"; break;
        }
        if (errmsg) {
            printf("%s\n", errmsg);
        }
        else {
            printf("Connection failed with rc=0x%02 (%u)", connrc, connrc);
        }
        goto quit;
    }
    
    /* we now have a session */
    printf("Session established with %s:%s\n", 
        mqtc->broker_host, mqtc->broker_port);
    rc = 0;

quit:
    // wtof("%s: exit rc=%d", __func__, rc);
    return rc;
}
