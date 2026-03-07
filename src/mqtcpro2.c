#include "mqtc370.h"

int mqtc_process_connack(MQTC *mqtc, CPB *pb)
{
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
    
    // wtof("%s: connflags=%02X (%u) connrc=%02X (%u)", 
    //     __func__, connflags, connflags, connrc, connrc);
    
    /* Are we waiting for a response? */
    if (!(mqtc->flags & MQTC_FLAG_WAITRESP)) {
        /* no, we're not waiting for a response */
        wtof("%s: session is NOT waiting", __func__);
        goto quit;
    }

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
            wtof("MQTC2000E %s", errmsg);
        }
        else {
            wtof("MQTC2000E Connection failed with rc=0x%02 (%u)", connrc, connrc);
        }
        mqtc->flags |= MQTC_FLAG_SHUTDOWN;
        goto quit;
    }
    
    /* we now have a session */
    wtof("MQTC2001I Session established with %s:%s", 
        mqtc->broker_host, mqtc->broker_port);
    /* reset flags */
    mqtc->flags &= ~(MQTC_FLAG_ERROR | MQTC_FLAG_SESSION | MQTC_FLAG_WAITRESP | MQTC_FLAG_RECONNECT);
    
    /* we're in session now */
    mqtc->flags |= MQTC_FLAG_SESSION;
    
    /* make sure we have a send packet to clear */
    pb = mqtc->spb;
    if (!pb) goto waitresp;
    if (pb->pos != pb->bufsize) goto waitresp;

    /* clear the current send packet */
    free(pb);
    mqtc->spb = NULL;
    // wtof("%s: send packet deleted", __func__);

waitresp:
    /* clear the wait response flag */
    mqtc->flags &= ~MQTC_FLAG_WAITRESP;
    // wtof("%s: wait response flag cleared", __func__);
    rc = 0;

quit:
    // wtof("%s: exit rc=%d", __func__, rc);
    return rc;
}
