#include "mqtb370.h"

static int user_on_list(MQTB *mqtb, MQTBC *mqtbc, MQTBS *mqtbs);
static int user_login_okay(MQTB *mqtb, MQTBC *mqtbc, MQTBS *mqtbs);

int mqtb_process_connect(MQTB *mqtb, MQTBS *mqtbs, PB *pb)
{
    MQTBC           *mqtbc  = mqtb->mqtbc;
    int             rc      = 0;
    unsigned        len     = 0;
    unsigned        pos     = 0;
    unsigned char   *uc;
    unsigned char   flags;

    if (!pb) goto quit;
    
    // mqtb_packet_dump(mqtb, mqtbs, pb, "CONNECT");

    /* decode variable header */
    if (!pb->varpos) {
        rc = util_length_decode(pb->buf, &len);
        if (!rc) goto quit;
        /* start of variable header */
        pb->varpos  = (unsigned short)rc;
    }
    pos = pb->varpos;
    
    /* Get protocol name length */
    len = util_get_ushort(&pb->buf[pos]);
    pos += 2;
    
    /* Get the protocol name as a string */
    mqtbs->protocal_name = util_dup_a2e(&pb->buf[pos], len);
    pos += len;

    // wtof("%s: Protocol Name=%s", __func__, mqtbs->protocal_name);

    /* Get the protocol level */
    mqtbs->proto    = pb->buf[pos++];
    // wtof("%s: Protocol Number=%u", __func__, mqtbs->proto);

    /* we're going to reject any "newer" protocols for now */
    if (mqtbs->proto != 4) { /* 4 == "3.1.1" */
        /* We don't speak that protocol (for now) */
        mqtbs->flags |= MQTBS_FLAG_ERROR;
        wtof("MQTTD304W Connection for protocol not equal 4 '3.1.1' (%u)", mqtbs->proto);
        rc = mqtb_reply_connack(mqtb, mqtbs, CONNACK_REFUSED_PROTOCOL);
        goto quit;
    }
    
    /* Get the connect flags */
    flags           = pb->buf[pos++];
    // wtof("%s: Flags=%02X", __func__, flags);

    mqtbs->will_retain = (flags & FLAG_WILL_RETAIN) ? 1 : 0;
    mqtbs->will_qos    = ((flags & FLAG_WILL_QOS) >> 3) & 0x03;
    mqtbs->will_send   = (flags & FLAG_WILL) ? 1 : 0;
    
    /* Get the Keep Alive */
    mqtbs->keepalive = util_get_ushort(&pb->buf[pos]);
    pos += 2;
    // wtof("%s: Keep Alive=%u", __func__, mqtbs->keepalive);

    if (mqtbs->proto == MQTBS_PROTO_500) {
        /* protocol version 5 can have properties */
        len = pb->buf[pos++];  /* length of properties */
        
        wtodumpf(&pb->buf[pos], len, "%s: Properties", __func__);
        
        /* we're going to ignore the properties for now. */
        pos += len;
    }

    /* save the start of the payload area */
    pb->paypos = pos;

    /* now we  decode the payload using the info from the variable header */
dopayload:
    
    /* get the length of the client id */
    len = util_get_ushort(&pb->buf[pos]);
    pos += 2;
    if (len) {
        // wtodumpf(&pb->buf[pos], len, "%s: clientid", __func__);
#if 0 /* this limit was removed by the MQTT 3.1.1 protocol */
        if (len > 23) {
            mqtbs->flags |= MQTBS_FLAG_ERROR;
            wtof("MQTTD305W Connection for client id > 23 bytes (%u)", len);
            rc = mqtb_reply_connack(mqtb, mqtbs, CONNACK_REFUSED_IDENTIFER);
            goto quit;
        }
#endif
        mqtbs->clientid = util_memdup(&pb->buf[pos], len);
        mqtbs->clientid_len = len;
        pos += len;
    }

    if (flags & FLAG_WILL) {
        /* get will topic */
        len = util_get_ushort(&pb->buf[pos]);
        pos += 2;
        if (len) {
            mqtbs->will_topic = util_dup_a2e(&pb->buf[pos], len);
            pos += len;
            // wtof("%s: Will Topic=%s", __func__, mqtbs->will_topic);
        }

        /* get will message */
        len = util_get_ushort(&pb->buf[pos]);
        pos += 2;
        if (len) {
            mqtbs->will_msg = util_memdup(&pb->buf[pos], len);
            mqtbs->will_msg_len = len;
            pos += len;
            // wtodumpf(mqtbs->will_msg, len, "%s: Will Message", __func__);
        }
    }
    
    if (flags & FLAG_USERID) {
        /* get userid */
        len = util_get_ushort(&pb->buf[pos]);
        pos += 2;
        if (len) {
            mqtbs->user = util_dup_a2e(&pb->buf[pos], len);
            util_str_upper(mqtbs->user);
            pos += len;
            // wtof("%s: Userid=%s", __func__, mqtbs->user);
        }
    }
    
    if (flags & FLAG_PASSWORD) {
        /* get password */
        len = util_get_ushort(&pb->buf[pos]);
        pos += 2;
        if (len) {
            mqtbs->password = util_dup_a2e(&pb->buf[pos], len);
            pos += len;
            // wtof("%s: Password=%s", __func__, mqtbs->password);
        }
    }

    if (mqtb->flags & MQTB_FLAG_QUIESCE) {
        wtof("MQTTD306I Connection rejected. Server is QUIESCE.");
        rc = mqtb_reply_connack(mqtb, mqtbs, CONNACK_REFUSED_SERVER);
        goto quit;
    }

    // wtof("%s: mqtbc->auth=0x%02X", __func__, mqtbc->auth);

    if (!mqtbs->user) {
        /* no userid */
        if (mqtbc->auth & MQTBC_AUTH_ANON) {
            /* anonymous allowed */
            goto okay;
        }
        mqtbs->flags |= MQTBS_FLAG_ERROR;
        wtof("MQTTD307W Connection rejected. ANONYMOUS user not allowed.");
        rc = mqtb_reply_connack(mqtb, mqtbs, CONNACK_REFUSED_NOT_AUTH);
        goto quit;
    }

    if (!user_on_list(mqtb, mqtbc, mqtbs)) {
        mqtbs->flags |= MQTBS_FLAG_ERROR;
        wtof("MQTTD308W Connection rejected. User not on auth_list.");
        rc = mqtb_reply_connack(mqtb, mqtbs, CONNACK_REFUSED_NOT_AUTH);
        goto quit;
    }

    if (!user_login_okay(mqtb, mqtbc, mqtbs)) {
        mqtbs->flags |= MQTBS_FLAG_ERROR;
        wtof("MQTTD309W Connection rejected. User login failed.");
        rc = mqtb_reply_connack(mqtb, mqtbs, CONNACK_REFUSED_USER_PASS);
        goto quit;
    }
    
okay:
    mqtbs->flags |= MQTBS_FLAG_SESSION;
    wtof("MQTTD301I Connection accepted %p", mqtbs);
    rc = mqtb_reply_connack(mqtb, mqtbs, CONNACK_ACCEPTED);

quit:
    /* we don't need the password after session validation */
    if (mqtbs->password) {
        len = strlen(mqtbs->password);
        memset(mqtbs->password, 0, len);
        util_free_char(&mqtbs->password);
    }
    return rc;
}

static int user_on_list(MQTB *mqtb, MQTBC *mqtbc, MQTBS *mqtbs)
{
    int         okay    = 1;
    unsigned    n, count;

    if (!(mqtbc->auth & MQTBC_AUTH_LIST)) goto quit;
    
    /* check userid against patterns in auth_list */
    count = array_count(&mqtbc->auth_list);
    for(n=0; n < count; n++) {
        char *pattern = mqtbc->auth_list[n];
        
        if (!pattern) continue;
        
        if (__patmat(mqtbs->user, pattern)) goto quit;
    }
    
    okay = 0;   /* not found in auth_list */

quit:
    return okay;
}


static int user_login_okay(MQTB *mqtb, MQTBC *mqtbc, MQTBS *mqtbs)
{
    int     okay    = 1;
    ACEE    *acee   = NULL;
    int     racf_rc = 0;

    if (!(mqtbc->auth & MQTBC_AUTH_LOGIN)) goto quit;

    /* authenticate user via RACF login */
    okay = 0;
    if (!mqtbs->password) {
        wtof("MQTTD311W Missing password for \"%s\".",
            mqtbs->user);
        goto quit;
    }

    acee = racf_login(mqtbs->user, mqtbs->password, NULL, &racf_rc);
    if (!acee) {
        wtof("MQTTD913E Login for \"%s\" failed. RACF rc=0x%08X (%d)",
            mqtbs->user, racf_rc, racf_rc);
        goto quit;
    }

    /* we don't need to stay logged in, discard ACEE */
    racf_rc = racf_logout(&acee);
    if (racf_rc) {
        wtof("MQTTD913W Logout for \"%s\" failed. RACF rc=0x%08X (%d)",
            mqtbs->user, racf_rc, racf_rc);
    }

    okay = 1;

quit:
    return okay;
}
