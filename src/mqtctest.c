#include "mqtc370.h"

static int 
sample_published_topic(MQTC *mqtc, const char *topic, const char *msg, unsigned len);

int main(int argc, char **argv)
{
    int         rc          = 0;
    MQTC        *mqtc       = NULL;
    int         i;

    mqtc = mqtc_new_client();
    wtof("%s: mqtc=%p", __func__, mqtc);
    if (!mqtc) goto quit;
#if 0
    rc = mqtc_open_client(mqtc, "mosquitto.rayborn.us", "1883");
    wtof("%s: mqtc_open_client(mqtc, \"mosquitto.rayborn.us\", \"1883\" rc=%d",
        __func__, rc);
#else
    rc = mqtc_open_client(mqtc, "tk5.rayborn.us", "1883");
    wtof("%s: mqtc_open_client(mqtc, \"tk5.rayborn.us\", \"1883\" rc=%d",
        __func__, rc);
#endif
    if (rc) goto quit;
    
    rc = mqtc_create_thread(mqtc);
    wtof("%s: mqtc_create_thread(mqtc) rc=%d", __func__, rc);
    if (rc) goto quit;

    rc = mqtc_set_userid(mqtc, "herc01");
    wtof("%s: mqtc_set_userid(mqtc, \"herc01\") rc=%d", __func__, rc);
    if (rc) goto quit;
    
    rc = mqtc_set_password(mqtc, "cul8tr");
    wtof("%s: mqtc_set_password(mqtc, \"cul8tr\") rc=%d", __func__, rc);
    if (rc) goto quit;
    
    rc = mqtc_connect(mqtc);
    wtof("%s: mqtc_connect(mqtc) rc=%d", __func__, rc);
    if (rc) goto quit;

    /* use this callback for published topics that we subscribe to */
    mqtc->f_rtopic = sample_published_topic;
#if 0    
    rc = mqtc_sub(mqtc, 0, "$SYS/#");
    wtof("%s: mqtc_sub(mqtc, 0, \"$SYS/#\") rc=%d", __func__, rc);
    if (rc) goto quit;
#endif    
    rc = mqtc_sub(mqtc, 2, "#");
    wtof("%s: mqtc_sub(mqtc, 2, \"#\") rc=%d", __func__, rc);
    if (rc) goto quit;
    
    for(i=1; i <= 10; i++) {
        rc = mqtc_pubf(mqtc, 2, 1, "mqtctest", "ticktock=%d", i);
        sleep(1);
        rc = mqtc_unsub(mqtc, "#");
    }

quit:
    mqtc_free_client(&mqtc);
    return rc;
}

static int 
sample_published_topic(MQTC *mqtc, const char *topic, const char *msg, unsigned len)
{
    int     i;
    char    *buf    = NULL;
    char    *p;
    
    wtof("%s: topic=\"%s\"", __func__, topic);
    if (msg) {
        if (msg[0]!='{') {
            wtof("msg=\"%*.*s\"", len, len, msg);
            goto quit;
        }
        
        buf = calloc(1, len + 2);
        if (!buf) goto quit;
        memcpy(buf, msg, len);
        
        wtof("msg={");
        for(i=0, p=strtok(buf, "{},"); p && *p; p=strtok(NULL, "{},"), i++) {
            if (i==0) {
                wtof("    %s", p);
            }
            else {
                wtof(",   %s", p);
            }
        }
        wtof("}");
    }
    
quit:
    if (buf) free(buf);
    return 0;
}
