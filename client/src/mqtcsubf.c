#include "mqtc370.h"

int mqtc_subf(MQTC *mqtc, unsigned qos, const char *fmt, ...)
{
    int         rc      = 0;
    va_list     args;
    int         i;
    int         len;
    char        buf[1024];

    va_start(args, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len >= sizeof(buf)) len = sizeof(buf) - 1;
    buf[len] = 0;

    rc = mqtc_sub(mqtc, qos, buf);

    return rc;
}
