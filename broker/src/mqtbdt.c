#include "mqtb370.h"

char *mqtb_fmt_datetime(MQTB *mqtb, time64_t *t64, char *timebuf)
{
    int         rc          = 0;
    MQTBC       *mqtbc;
    struct tm   *tm_info;

    if (!mqtb)      goto quit;
    if (!t64)       goto quit;
    if (!timebuf)   goto quit;
    
    mqtbc = mqtb->mqtbc;
    if (!mqtbc) goto quit;

    if (mqtbc->datetime & MQTBC_DATETIME_GMT) {
        tm_info = gmtime64(t64);
        strftime(timebuf, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    }
    else {
        tm_info = localtime64(t64);
        strftime(timebuf, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    }
    
quit:
    return timebuf;
}
