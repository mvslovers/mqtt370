#include "mqtb370.h"

int mqtb_pubv(MQTB *mqtb, const char *topic, const char *fmt, va_list args)
{
    int         rc      = 0;
    int         i;
    int         len;
    char        buf[1024];
    
    len = vsnprintf(buf, sizeof(buf), fmt, args);
    if (len >= sizeof(buf)) len = sizeof(buf) - 1;
    buf[len] = 0;

    // wtof("%s: topic=\"%s\" msg=\"%s\"", __func__, topic, buf);    

    for(i=0; i < len; i++) {
        buf[i] = utile2a[buf[i]];
    }

    rc = mqtb_pub(mqtb, topic, buf, len);

    return rc;
}
