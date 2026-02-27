#include "mqtc370.h"

int mqtc_pub(MQTC *mqtc, unsigned qos, unsigned retain, const char *topic, const char *msg)
{
    int         rc      = 0;
    unsigned    msglen  = msg ? strlen(msg) : 0;
    char        *buf    = msglen ? calloc(1, msglen) : NULL;

    /* convert message from EBCDIC to ASCII */
    if (buf) {
        unsigned n;
        for(n=0; n < msglen; n++) {
            buf[n] = utile2a[msg[n]];
        }
    }

    /* let mqtc_pub_bin() handle the rest */
    rc = mqtc_pub_bin(mqtc, qos, retain, topic, buf, msglen);
    
quit:
    if (buf) free(buf);
    return rc;
}
