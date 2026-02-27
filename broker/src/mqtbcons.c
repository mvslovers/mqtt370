#include "mqtb370.h"

static int process(MQTB *mqtb, char *buf);

int
mqtb_console(MQTB *mqtb, CIB *cib)
{
    int     rc      = 0;
    char    *buf    = NULL;

    if (!cib) goto quit;

    switch(cib->cibverb) {
    case CIBSTART:
        wtof("MQTTD100I CONS(%u) START", cib->cibconid);
        break;
    case CIBMODFY:
        wtof("MQTTD100I CONS(%u) \"%-*.*s\"",
            cib->cibconid, cib->cibdatln, cib->cibdatln, cib->cibdata);
        buf = calloc(1, cib->cibdatln + 2);
        if (buf) {
            memcpy(buf, cib->cibdata, cib->cibdatln);
            process(mqtb, buf);
        }
        break;
    case CIBSTOP:
        wtof("MQTTD100I CONS(%u) STOP", cib->cibconid);
        rc = 1;
        break;
    case CIBMOUNT:
        wtof("MQTTD100I CONS(%u) MOUNT", cib->cibconid);
        break;
    }

quit:
    if (buf) free(buf);
    return rc;
}

static int process(MQTB *mqtb, char *buf)
{
    return 0;
}
