#include "mqtb370.h"

/* The functions below are used to save and load retained topic
 * messages.
 * 
 * Note: The method used is to treat each retained record as a 
 * stream of bytes with the first 4 bytes being the length of the
 * entire record. The header has offsets to the topic and message
 * areas within the retained record.
 */

typedef struct mqtb_retain_hdr  MQRH;   /* retain header            */
typedef struct mqtb_retain_top  MQRT;   /* retain topic data        */
typedef struct mqtb_retain_msg  MQRM;   /* retain message data      */

/* Retain Records */
struct mqtb_retain_hdr {
    unsigned        len;            /* 00 length of record          */
    unsigned char   qos;            /* 04 QOS level (0,1,2)         */
    unsigned char   o_topic;        /* 05 offset to topic           */
    unsigned short  o_msg;          /* 06 offset to message         */
    time64_t        expires;        /* 08 expires after this time   */
    unsigned short  msgid;          /* 10 message id (QOS 1,2 only) */
};                                  /* 12 (18 bytes)                */

struct mqtb_retain_top {
    unsigned short  len;            /* 00 length of topic           */
    unsigned char   topic[0];       /* 02 topic data                */
};

struct mqtb_retain_msg {
    unsigned        len;            /* 00 length of message         */
    unsigned char   msg[0];         /* 04 message data              */
};

#define BUFSIZE  sizeof(MQRH) +     /* header area       */ \
                 sizeof(MQRT) +     /* topic area        */ \
                 sizeof(MQRM) +     /* message area      */ \
                 MAX_TOPIC    +     /* max topic name    */ \
                 MAX_MESSAGE        /* max message data  */  
    
static int save_topic(FILE *fp, MQPUB *mqpub, char *buf);

int mqtb_retain_save(MQTB *mqtb, const char *dataset)
{
    int             rc      = 8;
    int             lockrc  = 8;
    unsigned        records = 0;
    unsigned char   *buf    = NULL;
    FILE            *fp     = NULL;
    char            fn[56];
    unsigned        n, count;

    buf = calloc(1, BUFSIZE);
    if (!buf) {
        wtof("MQTTD420E Out of memory. Size=%u", BUFSIZE);
        goto quit;
    }

    /* quote the dataset name to to force fopen() to use it as-is */
    snprintf(fn, sizeof(fn), "'%s'", dataset);

    /* this dataset *should* be recfm=vb as we're using it to store 
     * our records as a stream of bytes. the dataset lrecl and blksize
     * could be any size as we're not doing record I/O.
     */
    fp = fopen(fn, "wb,recfm=vb,lrecl=27994,blksize=27998");
    
    if (!fp) {
        wtof("MQTTD421E Unable to open %s for output", fn);
        goto quit;
    }

    lockrc = lock(&mqtb->topics, LOCK_EXC);

    wtof("MQTTD422I Saving retained topics to %s", fn);

    rc = 0;
    count = array_count(&mqtb->topics);
    for(n=0; n < count; n++) {
        MQPUB *mqpub = mqtb->topics[n];
        
        if (!mqpub) continue;
        rc = save_topic(fp, mqpub, buf);
        if (rc) break;
        records++;
    }

    wtof("MQTTD429I %u topics saved to %s", records, fn);
    
quit:
    if (lockrc==0) unlock(&mqtb->topics, LOCK_EXC);
    if (fp) fclose(fp);
    if (buf) free(buf);
    return rc;
}

static int save_topic(FILE *fp, MQPUB *mqpub, char *buf)
{
    int         rc      = 8;
    MQRH        *mqrh   = (MQRH*)buf;
    MQRT        *mqrt   = NULL;
    MQRM        *mqrm   = NULL;
    unsigned    pos     = 0;
    unsigned    maxmsg  = 0;

    mqrh->len           = 0;
    mqrh->qos           = mqpub->qos;
    mqrh->o_topic       = sizeof(MQRH);
    mqrh->o_msg         = 0;
    mqrh->expires       = mqpub->expires;
    mqrh->msgid         = mqpub->msgid;
    pos                 = sizeof(MQRH);

    mqrt                = (MQRT*)&buf[pos];
    mqrt->len           = strlen(mqpub->topic);
    memcpy(mqrt->topic, mqpub->topic, mqrt->len);
    pos                 += sizeof(MQRT) + mqrt->len;

    if (!mqpub->msg_len) goto saveit;

    mqrh->o_msg         = pos;
    mqrm                = (MQRM*)&buf[pos];
    mqrm->len           = mqpub->msg_len;
    memcpy(mqrm->msg, mqpub->msg, mqpub->msg_len);
    pos                 += sizeof(MQRM) + mqrm->len;

saveit:
    mqrh->len           = pos;
    // wtodumpf(buf, pos, "SAVE TOPIC");

    maxmsg = fwrite(buf, 1, pos, fp);

    if (maxmsg != pos) {
        wtof("MQTBD423E write error.");
        goto quit;
    }

    rc = 0;

quit:
    return rc;
}

int mqtb_retain_load(MQTB *mqtb, const char *dataset)
{
    int             rc      = 8;
    int             lockrc  = 8;
    unsigned        records = 0;
    unsigned char   *buf    = NULL;
    FILE            *fp     = NULL;
    MQRH            *mqrh;
    MQRT            *mqrt;
    MQRM            *mqrm;
    MQPUB           *mqpub;
    unsigned        pos;
    char            fn[56];
    unsigned        n, count;
    time64_t        now;

    buf = calloc(1, BUFSIZE);
    if (!buf) {
        wtof("MQTTD400E Out of memory. Size=%u", BUFSIZE);
        goto quit;
    }
    mqrh = (MQRH*)buf;

    /* quote the dataset name to to force fopen() to use it as-is */
    snprintf(fn, sizeof(fn), "'%s'", dataset);

    fp = fopen(fn, "rb");
    if (!fp) {
        wtof("MQTTD401I Unable to open %s for input", fn);
        goto quit;
    }

    lockrc = lock(&mqtb->topics, LOCK_EXC);

    wtof("MQTTD402I Loading retained topics from %s", fn);

    /* delete any existing topics */
    count = array_count(&mqtb->topics);
    for(n=count; n > 0; n--) {
        MQPUB *mqpub = array_del(&mqtb->topics, n);
        
        if (!mqpub) continue;

        /* delete old topic */
        mqtb_publish_free(mqtb, &mqpub);
    }

    time64(&now);
    for(;;) {
        /* get the record length */
        n = fread(mqrh, 1, sizeof(mqrh->len), fp);
        // wtodumpf(mqrh, n, "mqrh->len=%u", mqrh->len);
        if (feof(fp)) break;
        if (n!=sizeof(mqrh->len)) {
            wtof("MQTTD403E Read error. Short record.");
            goto quit;
        }
        count = mqrh->len;
        if (count < sizeof(MQRH)) {
            wtof("MQTTD404E Invalid record size %u", count);
            goto quit;
        }
        if (count > BUFSIZE) {
            wtof("MQTTD405E Invalid record size %u", count);
            goto quit;
        }
        count   -= sizeof(mqrh->len);
        pos     = sizeof(mqrh->len);
        n = fread(&buf[pos], 1, count, fp);
        // wtodumpf(&buf[pos], n, "&buf[%u] count=%u", pos, count);
        if (n!=count) {
            wtof("MQTTD406E Read error. Short record.");
            goto quit;
        }

        mqrt = (MQRT*)&buf[mqrh->o_topic];
        if (mqrt->len == 0 || mqrt->len > MAX_TOPIC) {
            wtof("MQTTD407E Invalid topic length %u", mqrt->len);
            goto quit;
        }

        if (__64_cmp(&mqrh->expires, &now)==__64_SMALLER) {
            mqrt->topic[mqrt->len] = 0;
            wtof("MQTTD408I Discarding expired topic=\"%s\"", mqrt->topic);
            continue;
        }
        
        mqrm = NULL;
        if (mqrh->o_msg) {
            mqrm = (MQRM*)&buf[mqrh->o_msg];
            if (mqrm->len==0) mqrm = NULL;
        }

        /* this *should* be a complete retain record */
        mqpub = calloc(1, sizeof(MQPUB));
        if (!mqpub) {
            wtof("MQTTD409E Out of memory. Size=%u", sizeof(MQPUB));
            goto quit;
        }

        strcpy(mqpub->eye, MQPUB_EYE);
        mqpub->retain       = 1;
        mqpub->qos          = mqrh->qos;
        mqpub->expires      = mqrh->expires;
        mqpub->msgid        = mqrh->msgid;
        mqpub->topic        = calloc(1, mqrt->len + 1);
        if (!mqpub->topic) {
            wtof("MQTTD410E Out of memory. Size=%u", mqrt->len+1);
            goto quit;
        }
        memcpy(mqpub->topic, mqrt->topic, mqrt->len);

        if (mqrm) {
            mqpub->msg      = calloc(1, mqrm->len);
            if (!mqpub->msg) {
                wtof("MQTTD411E Out of memory. Size=%u", mqrm->len);
                goto quit;
            }
            memcpy(mqpub->msg, mqrm->msg, mqrm->len);
            mqpub->msg_len = mqrm->len;
        }

        array_add(&mqtb->topics, mqpub);
        records++;
    }

    wtof("MQTTD412I %u topics loaded from %s", records, fn);
    rc = 0;
    
quit:
    if (lockrc==0) unlock(&mqtb->topics, LOCK_EXC);
    if (fp) fclose(fp);
    if (buf) free(buf);
    return rc;
}
