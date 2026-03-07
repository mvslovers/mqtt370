#include "mqtc370.h"

MQTC *mqtc_init_client(MQTC *mqtc)
{
    if (mqtc) {
        memset(mqtc, 0, sizeof(MQTC));
        strcpy(mqtc->eye, MQTC_EYE);

        mqtc->sock      = -1;
        mqtc->keepalive = 60;
        
        /* if we have a DD:PKTTRC dataset, open it for writing */
        mqtc->pkttrc    = fopen("DD:PKTTRC", "w");
        if (mqtc->pkttrc) {
            time64_t now;
            time64(&now);
            fprintf(mqtc->pkttrc, " MQTT Client Side Packet Trace %s", ctime64(&now));
        }
    }
    
    return mqtc;
}
