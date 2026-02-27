#include "mqtb370.h"

int mqtb_topic_is_valid(const char *topic)
{
    int     is_valid    = 0;    /* not valid */
    int     len;
    
    if (!topic) goto quit;

    if (topic[0] == '$') goto quit; /* reserved for broker use only */
    
    for(len = 0; topic[len] ; len++) {
        if (topic[len] == '+') goto quit;
        if (topic[len] == '#') goto quit;
    }
    
    if (len > MAX_TOPIC) goto quit;

    is_valid = len;   /* valid topic */

quit:
    return is_valid;
}
