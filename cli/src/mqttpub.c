#include "mqttcli.h"

static int publish_cb(MQTC *mqtc, CPB *pb);

int mqtt_pub(MQTTCLI *mqttcli)
{
    int         rc      = 8;
    MQTC        *mqtc   = mqttcli->mqtc;
    int         sock    = mqtc ? mqtc->sock : -1;
    void        *rcpb   = mqtc->f_rcpb;
    unsigned    wait;   /* 1 = .01 seconds */
    int         tries;

    // printf("%s enter\n", __func__);

    if (!mqttcli->host) {
        printf("Missing host name\n");
        goto quit;
    }
    
    if (!mqttcli->port) {
        printf("Missing port number\n");
        goto quit;
    }

    if (!mqttcli->topic) {
        printf("Missing publish topic\n");
        goto quit;
    }
    
    if (sock < 0 || !(mqttcli->flags & MQTTCLI_FLAG_CONN)) {
        /* attempt connection to broker */
        rc = mqtt_connect(mqttcli);
        if (rc) goto quit;
    }


    /* add callback for PUBLISH response */
    mqtc->f_rcpb = publish_cb;
    mqtc->ctx    = mqttcli;
    mqttcli->ecb = 0;

    /* send CONNECT request */
    rc = mqtc_pub(mqttcli->mqtc, mqttcli->qos, mqttcli->retain, mqttcli->topic, mqttcli->msg);
    if (rc) {
        printf("Error sending PUBLISH rc=%d\n", rc);
        goto quit;
    }
        
    for(tries=0, rc=1; tries < 10 && rc == 1; tries++) {
        if (mqttcli->flags & MQTTCLI_FLAG_POLL) {
            rc = mqtc_send_recv(mqtc);
            // printf("mqtc_send_recv(mqtc) rc=%d\n", rc);
            if (rc) {
                printf("Send/Recv failed\n");
                goto quit;
            }
        }
        
        if (mqttcli->qos==0) {
            /* there is no response for QOS 0 */
            if (!mqtc->spb && array_count(&mqtc->spbq)==0) break;
        }
        
        /* set the wait time for 0.5 seconds */
        wait = 50;

        /* wait for ecb post or timer expire */
        rc = cthread_timed_wait(&mqttcli->ecb, wait, 1);
        // printf("cthread_timed_wait(&mqttcli->ecb, wait, 1) rc=%d\n", rc);
    }

    if (rc) {
        printf("Publish failed. Timed Out\n");
    }

quit:
    mqtc->f_rcpb = rcpb;
    // printf("%s exit rc=%d\n", __func__, rc);
    return rc;
}

static int publish_cb(MQTC *mqtc, CPB *pb)
{
    int         rc          = 0;
    MQTTCLI     *mqttcli    = mqtc->ctx;
    unsigned    type;
    
    type = (pb->buf[0] & 0xF0);
    switch (type) {
    case TYPE_PUBACK:   /* QOS 1 response */
        /* post waiting thread */
        cthread_post(&mqttcli->ecb, 0);
        break;
    case TYPE_PUBCOMP:  /* QOS 2 response */
        /* post waiting thread */
        cthread_post(&mqttcli->ecb, 0);
        break;
    }

quit:
    return rc;
}
