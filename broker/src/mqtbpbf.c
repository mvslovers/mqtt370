#include "mqtb370.h"

int mqtb_pubf(MQTB *mqtb, const char *topic, const char *fmt, ...)
{
    int         rc      = 0;
    va_list     args;
    
    va_start(args, fmt);
    rc = mqtb_pubv(mqtb, topic, fmt, args);
    va_end(args);

    return rc;
}
