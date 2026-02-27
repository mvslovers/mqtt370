#include "mqtb370.h"

void mqtbc_free(MQTBC **pmqtbc)
{
    MQTBC       *mqtbc;
    unsigned    n, count;
    
    if (!pmqtbc) goto quit;
    mqtbc = *pmqtbc;
    if (!mqtbc) goto quit;
    
    util_free_char(&mqtbc->tz);

    if (mqtbc->auth_list) {
        count = array_count(&mqtbc->auth_list);
        for(n=count; n > 0; n--) {
            char *p = mqtbc->auth_list[n];
            if (!p) continue;
            free(p);
        }
        array_free(&mqtbc->auth_list);
    }
    
    free(mqtbc);
    *pmqtbc = NULL;

quit:
    return;
}
